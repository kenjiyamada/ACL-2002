#include "decode3.h"

Decoder::Decoder(LM& lm, TM& tm, CFG& cfg) : lm(lm), tm(tm), cfg(cfg)
{
  // init LMInfo class
  LMInfo::classInit(lm);

  // init vars
  topSymbol = sym("TOP");
  symX = sym("X");

#ifdef CHEAT_INFO
  readCheatInfo();
#endif

  // alloc constits
  allocConstitVecs();
}

void
Decoder::allocConstitVecs()
{
  for (int i=0; i<maxSentLen; i++)
    for (int j=0; j<maxSentLen; j++) {
      constitsP[i][j] = new vector<Constit *>;
      constitsC[i][j] = new vector<Constit *>;
      constitsX[i][j] = new vector<Constit *>;
      constitsC2[i][j] = new vector<Constit *>;
    }
}

void
Decoder::DecodeInputs(istream& in)
{
  // decode multiple sentences
  string line,w; vector<string> item;
  while(getline(in,line)) {
    istrstream ist(line.c_str());
    item.clear();
    while(ist>>w) item.push_back(w);
    if (item.size()>0) {
      if (item[0]=="#Prob:") {
	Params::SetThreshold(item[1],item[2],item[3],item[4]);
	Params::Print();
	resetRules();
      } else {
	if (rulesEmpty()) resetRules(); // for the first time.
	decodeIt(item);
      }
    }
  }
}

void 
Decoder::showResults()
{
  // came from Decode2/decoder.cc

  cout << "Decoder Results:\n";
  int n=1;
  vector<Constit *>& vec = getConstitVecP(0,sentLen());
  sort(vec.begin(),vec.end(),ConstitLt);
  for (int i=0; i<vec.size(); i++) {
    Constit *c = vec[i];
    if (c->label == topSymbol) {
      cout << "Result " << n++ << ": ";
      string esent;
      c->GetOrgEWords(esent);
      cout << esent;
      ShowCost(c->tmcost,c->lmcost());
      //if (Params::verbose) c->ShowOrg();
      if (Params::cheatShow) {
	cout << "CheatInfo:\n";
	showCheat(c);
      }
      if (Params::showTree) {
	showTree(c->children[0]);
	cout << "\n";
      }
    }
  }
}


void
Decoder::clear()
{
  clearChart();
}

void
Decoder::clearChart()
{
  for (int i=0; i<maxSentLen; i++)
    for (int j=0; j<maxSentLen; j++) {
      vector<Constit *>& vecP = getConstitVecP(i,j);
      for (int k=0; k<vecP.size(); k++) delete vecP[k];
      vecP.clear();

      vector<Constit *>& vecC = getConstitVecC(i,j);
      for (int k=0; k<vecC.size(); k++) delete vecC[k];
      vecC.clear();

      vector<Constit *>& vecX = getConstitVecX(i,j);
      for (int k=0; k<vecX.size(); k++) delete vecX[k];
      vecX.clear();
    }
}

//
// Decoder high-level functions
//

void
Decoder::decodeIt(vector<string>& words)
{
  // copy sentence to parse
  jwords=words;

  // clear old stuff
  clear();

  // initialize chart and go parse loop
  mainLoop();
  showResults();
}

void
Decoder::showChartLen(Pos start, Pos end, string msg)
{
  if (Params::verbose) {
    int lenP = getConstitVecP(start,end).size();
    int lenC = getConstitVecC(start,end).size();
    int lenX = getConstitVecX(start,end).size();
    int lenC2 = getConstitVecC2(start,end).size();
    cout << msg << "[" << lenP << "," << lenC << "," << lenC2 << "," << lenX << "]";
    cout.flush();
  }
}

void
Decoder::mainLoop()
{
  initBaseLexConstits();
  initZeroLengthConstits();	// zero-length constits are stored separately
  initBaseLM();			// set LMInfo
  
  for (Pos len=1; len<=sentLen(); len++) {
    for (Pos start=0; start<=sentLen()-len; start++) {
      Pos end=start+len;
      if (Params::verbose) {
	cout << "MainLoop(" << int(start) << "," << int(end) << ")\t";
	cout << "\n"; Constit::showAlloc();
      }
      //if (len>4) return;	// DEBUG
      applyNonUnaryRules(start,end);

      showChartLen(start,end,"NonUnary:");
      ChartInfo range(this,start,end);
      pruneConstits(start,end,range);	// must be done before unary/zero-length (set range too)
      range.set();
      showChartLen(start,end," Pruned:");


      for (int i=0; i<maxUnary; i++) {
	// these has to be applied to newly created constits only
	// NYI: only constits with cost < min+beam can be added.
	applyUnaryRules(start,end,range);
	addZeroLengthConstits(start,end,range);
	if (range.unchanged()) break;
	range.update();
      }
      showChartLen(start,end," Unary:");
      if (Params::verbose) cout << "\n"; // to flush output from showChartLen()
#ifdef CHEAT_INFO
      inspectCheatInfo(start,end);
#endif
    }
  }
}

// C2-constit
//    ex. P -> A B C  ==> P -> (A B) C
//    cost: add rule cost for the first C2-constit, plus sum of all children.
//          lmcost will be recalculated at the end.

void
Decoder::applyNonUnaryRules(Pos start, Pos end)
{
#if 0
  // for(all partitions of [start,end]) C-constits (by P-rule) => P-constit
  // (including zero-length partitions, but it must be less than end-start.
  // also exclude unary partition)

  tryAllPartitions(start,end);
#else
  // Originally try all partitions of [start,end] for
  // C-constit + C-constit (by P-rule) => P-constit.
  // Now, more than 2-ary (binary) rules will create a C2-constit as an open edge.

  // Currently, zero-length edge is not considered. (To be added later)
  
  for (int split=start+1; split<=end-1; split++) {
    combineCC(start,split,end);	// C-constit + C-constit
    combineC2C(start,split,end); // C2-constit + C-constit
  }
#endif  

  // for(all possible binary paritions) P-constits + X-constits (by C-rule) => C-constit
  // (length of X-constits >=1. (for now ==1))

  for (int i=1; i<=maxXConstitLen; i++) {
    combineXP(start,start+i,end);
    combinePX(start,end-i,end);
  }

}

void
Decoder::applyUnaryRules(Pos start, Pos end, ChartInfo& range)
{
  // NYI: only constits with cost < min+beam can be added.

  // for(all unary P-rules) C-constit + P-rule => P-constit
  applyUP(start,end,range);

  // for(all unary C-rules) P-constit + C-rule => C-constit
  applyUC(start,end,range);
}

void
Decoder::addZeroLengthConstits(Pos start, Pos end, ChartInfo& range)
{
  // NYI: only constits with cost < min+beam can be added.

  // append (or prepend) zero-length constit

  // zero-length C-constit to C-constit[start,end] (by P-rule) => P-constit
  // (binary/trinary P-rules only)
  appendZC(start,end);

  // zero-length P-constit + X-constit[start,end] (by C-rule) => C-constit
  appendZP(start,end);
}

//
// Decoder subroutines
//

void 
Decoder::initBaseLexConstits()
{
  // create L-constits (excluding zero-length constits)

  for (int pos=0; pos<sentLen(); pos++) 
    createLexConstits(jwords[pos],pos,1);
}

void 
Decoder::initZeroLengthConstits()
{
  if (!Params::noNullJWords) {
    // create zero-length L-constits at [0,0]
    // expand with P-rules/C-rules (as in mainLoop)
    // copy to all [n,n] position (?)
  }
}

void 
Decoder::createLexConstits(const string& jword, int pos, int len)
{
  // obtain all J->E words, and add to P-constits[pos,pos+len]
  // (from decoder.cc:Decoder::addLexKeys2()

  vector<string> ewords; vector<double>tprobs;
  vector<Symbol> lbls;   vector<double>cprobs;

  // obtain J->E
  tm.GetEWords(jword,ewords,tprobs);

  // For each eword
  for (int ei=0; ei<ewords.size(); ei++) {
    cfg.GetLexProbs(ewords[ei],lbls,cprobs);
    // For each POS
    for (int ni=0; ni<lbls.size(); ni++) {
      double cost = -log(tprobs[ei]*cprobs[ni]);
      addLexConstit(lbls[ni],pos,pos+len,ewords[ei],cost);
    }
  }
}


void Decoder::initBaseLM() { }


void 
Decoder::applyUP(Pos start, Pos end, ChartInfo& range)
{
  // for(all unary P-rules) C-constit + P-rule => P-constit
  vector<Constit *>& vec = getConstitVecC(start,end);
  for (int i=range.fromC; i<range.toC; i++) {
    Constit *co = vec[i];
    Rule *r = rb.GetUnaryRuleP(co->label);
    if (r && unaryWithinBeamP(co,r->cost,range)) {
      //vector<Constit *> ch; ch.push_back(co);
      Kids ch; ch.push_back(co);
      double cost = co->tmcost + r->cost;
      addConstitP(r->lhs,start,end,r,ch,cost);
    }
  }
}

void 
Decoder::applyUC(Pos start, Pos end, ChartInfo& range)
{
  // for(all unary C-rules) P-constit + C-rule => C-constit
  vector<Constit *>& vec = getConstitVecP(start,end);
  for (int i=range.fromP; i<range.toP; i++) {
    Constit *co = vec[i];
    vector<Rule *>& r = rb.GetUnaryRulesC(co->label);
    for (int j=0; j<r.size(); j++) {
      if (unaryWithinBeamC(co,r[j]->cost,range)) {
	//vector<Constit *> ch; ch.push_back(co);
	Kids ch; ch.push_back(co);
	double cost = co->tmcost + r[j]->cost;
	addConstitC(r[j]->lhs,start,end,r[j],ch,cost);
      }
    }
  }
}

inline bool
Decoder::unaryWithinBeamP(Constit *c, double extcost, ChartInfo& range)
{
  double min,cost;
  if (c->lmcost()==0) {
    min = range.minP0; cost = c->tmcost + extcost;
  } else {
    min = range.minP1; cost = c->tmcost + c->lmcost() + extcost;
  }
  return (extcost < min + Params::pruneBeam);
}

inline bool
Decoder::unaryWithinBeamC(Constit *c, double extcost, ChartInfo& range)
{
  double min,cost;
  if (c->lmcost()==0) {
    min = range.minC0; cost = c->tmcost + extcost;
  } else {
    min = range.minC1; cost = c->tmcost + c->lmcost() + extcost;
  }
  return (extcost < min + Params::pruneBeam);
}

void 
Decoder::combineXP(Pos start, Pos split, Pos end)
{
  // X-constit[start,split] + P-constit[split,end] (by C-rule) => C-constit
  vector<Constit *>& vecX = getConstitVecX(start,split);
  vector<Constit *>& vecP = getConstitVecP(split,end);

  for (int i=0; i<vecX.size(); i++) {
    if (vecX[i]->label==symX) {
      for (int j=0; j<vecP.size(); j++) {
	vector<Rule *>& rules = rb.GetRulesXP(vecP[j]->label);
	for (int k=0; k<rules.size(); k++) {
	  Kids ch;
	  ch.push_back(vecX[i]); ch.push_back(vecP[j]);
	  double cost = vecX[i]->tmcost + vecP[j]->tmcost + rules[k]->cost;
	  addConstitC(rules[k]->lhs,start,end,rules[k],ch,cost);
	}
      }
    }  
  }
}

void 
Decoder::combinePX(Pos start, Pos split, Pos end)
{
  // P-constit[start,split] + X-constit[split,end] (by C-rule) => C-constit
  vector<Constit *>& vecP = getConstitVecP(start,split);
  vector<Constit *>& vecX = getConstitVecX(split,end);

  for (int i=0; i<vecX.size(); i++) {
    if (vecX[i]->label==symX) {
      for (int j=0; j<vecP.size(); j++) {
	vector<Rule *>& rules = rb.GetRulesPX(vecP[j]->label);
	for (int k=0; k<rules.size(); k++) {
	  Kids ch;
	  ch.push_back(vecP[j]); ch.push_back(vecX[i]);
	  double cost = vecX[i]->tmcost + vecP[j]->tmcost + rules[k]->cost;
	  addConstitC(rules[k]->lhs,start,end,rules[k],ch,cost);
	}
      }
    }  
  }
}

void
Decoder::combineCC(Pos start, Pos split, Pos end)
{
  // C-constit[start,split] + C-constit[split,end] (by P-rule) => P-constit or C2-constit
  vector<Constit *>& v1 = getConstitVecC(start,split);
  vector<Constit *>& v2 = getConstitVecC(split,end);

  for (int i=0; i<v1.size(); i++) {
    Symbol s1 = v1[i]->label;
    for (int j=0; j<v2.size(); j++) {
      Symbol s2 = v2[j]->label;
      const vector<Rule *> *rv = rb.GetNonUnaryRulesP2(s1,s2);
      if (rv) {
	Constit *c1 = v1[i], *c2 = v2[j];
	Kids ch; ch.push_back(c1); ch.push_back(c2);
	for (int k=0; k<rv->size(); k++) {
	  Rule *r = (*rv)[k];
	  if (r->rhs.size()>2) {
	    // create C2-constit
	    //double cost = c1->tmcost + c1->lmcost() + c2->tmcost + c2->lmcost() + r->cost;
	    //double cost = c1->tmcost + c2->tmcost + r->cost;  // no LMcost
	    double tmcost = c1->tmcost + c2->tmcost + r->cost;
	    double lmcost = c1->lmcost() + c2->lmcost();
	    addConstitC2(r->lhs, start, end, r, ch, tmcost,lmcost);
	  } else {
	    // create P-constit
	    double cost = c1->tmcost + c2->tmcost + r->cost;
	    addConstitP(r->lhs, start, end, r, ch, cost);
	  }
	}
      }
    }
  }
}

void
Decoder::combineC2C(Pos start, Pos split, Pos end)
{
  // C2-constit + C-constit (by P-rule) => P-constit or C2-constit
  vector<Constit *>& v1 = getConstitVecC2(start,split);
  vector<Constit *>& v2 = getConstitVecC(split,end);

  for (int i=0; i<v1.size(); i++) {
    Constit *c2 = v1[i];
    int numc = c2->children.size();
    Symbol dot = c2->rule->rhs[numc];
    for (int j=0; j<v2.size(); j++) {
      Constit *c = v2[j];
      if (c->label == dot) {
	if (numc+1==c2->rule->rhs.size()) {
	  // create P-constit
	  Kids ch(c2->children); ch.push_back(c);
	  double cost = c2->rule->cost;
	  for (int k=0; k<ch.size(); k++) cost += ch[k]->tmcost;
	  addConstitP(c2->label,c2->start,end,c2->rule,ch,cost);
	} else {
	  // expand C2-constit
	  //double addcost = c->tmcost + c->lmcost();
	  //double addcost = c->tmcost;  // no LMcost
	  //expandConstitC2(c2,c,end,addcost);
	  expandConstitC2(c2,c,end);
	}
      }
    }
  }
}

void 
Decoder::tryAllPartitions(Pos start, Pos end)
{
  // for(all partitions of [start,end]) C-constits (by P-rule) => P-constit
  // (including zero-length partitions, but it must be less than end-start.
  // also exclude unary partition)

  // s1,s2,... are split points

  int maxlen=end-start;
  for (int s1=start; s1<end; s1++) {
    if (Params::verbose) {
      cout << "tryAllPartitions(" << int(start) << "," << int(end) << ")[s1=" << s1 << "]\n";
      Constit::showAlloc();
      //if (start==2 && end==7 && s1==3) Params::debug=1;
      if (start==2 && end==7 && s1==4) Params::debug=0;
    }
    vector<Constit *>& cv1 = getConstitVecC(start,s1);
    for (int j1=0; j1<cv1.size(); j1++) {
      if (Params::debug) { cout << "[j1=" << j1 << "]"; Constit::showAlloc(); }
      Constit *c1 = cv1[j1]; //string k1 = sym(c1->label);
      RHS k1; k1.push_back(c1->label);
      // try binary 
      if (end-s1<maxlen) {
	vector<Constit *>& cv2 = getConstitVecC(s1,end);
	for (int j2=0; j2<cv2.size(); j2++) {
	  Constit *c2 = cv2[j2]; //string k2 = k1 + " " + sym(c2->label);
	  RHS k2(k1); k2.push_back(c2->label);
	  tryCreateConstit(c1,c2,k2);
	}
      }
      // set second split point (s2)
      for (int s2=s1; s2<=end; s2++) {
	if (Params::debug) { cout << "[s2=" << s2 << "]"; Constit::showAlloc(); }
	vector<Constit *>& cv2 = getConstitVecC(s1,s2);
	for (int j2=0; j2<cv2.size(); j2++) {
	  Constit *c2 = cv2[j2]; //string k2 = k1 + " " + sym(c2->label);
	  RHS k2(k1); k2.push_back(c2->label);
	  // try 3-ary
	  if (end-s2<maxlen) {
	    vector<Constit *>& cv3 = getConstitVecC(s2,end);
	    for (int j3=0; j3<cv3.size(); j3++) {
	      Constit *c3 = cv3[j3]; //string k3 = k2 + " " + sym(c3->label);
	      RHS k3(k2); k3.push_back(c3->label);
	      tryCreateConstit(c1,c2,c3,k3);
	    }
	  }
	  // set third split point (s3)
	  for (int s3=s2; s3<=end; s3++) {
	    //if (Params::debug) { cout << "[s3=" << s3 << "]"; Constit::showAlloc(); }
	    vector<Constit *>& cv3 = getConstitVecC(s2,s3);
	    for (int j3=0; j3<cv3.size(); j3++) {
	      Constit *c3 = cv3[j3]; //string k3 = k2 + " " + sym(c3->label);
	      RHS k3(k2); k3.push_back(c3->label);
	      // try 4-ary
	      if (end-s3<maxlen) {
		vector<Constit *>& cv4 = getConstitVecC(s3,end);
		for (int j4=0; j4<cv4.size(); j4++) {
		  Constit *c4 = cv4[j4]; //string k4 = k3 + " " + sym(c4->label);
		  RHS k4(k3); k4.push_back(c4->label);
		  tryCreateConstit(c1,c2,c3,c4,k4);
		}
	      }
	      // set fourth (last) split point (s4)
	      for (int s4=s3; s4<=end; s4++) {
		vector<Constit *>& cv4 = getConstitVecC(s3,s4);
		for (int j4=0; j4<cv4.size(); j4++) {
		  Constit *c4 = cv4[j4]; //string k4 = k3 + " " + sym(c4->label);
		  RHS k4(k3); k4.push_back(c4->label);
		  // try 5-ary (last)
		  if (end-s4<maxlen) {
#if 1
		    vector<Constit *>& cv5 = getConstitVecC(s4,end);
		    for (int j5=0; j5<cv5.size(); j5++) {
		      Constit *c5 = cv5[j5]; //string k5 = k4 + " " + sym(c5->label);
		      RHS k5(k4); k5.push_back(c5->label);
		      tryCreateConstit(c1,c2,c3,c4,c5,k5);
		    }
#else
		    vector<Constit *> *cv5 = constitsC[s4][end]; // bug=P
		    for (int j5=0; j5<cv5->size(); j5++) {
		      Constit *c5 = (*cv5)[j5]; //string k5 = k4 + " " + sym(c5->label);
		      RHS k5(k4); k5.push_back(c5->label);
		      tryCreateConstit(c1,c2,c3,c4,c5,k5);
		    }
#endif

		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }
}

inline void 
Decoder::tryCreateConstit(Constit *c1, Constit *c2, RHS& key)
{
  const vector<Rule *> *rv = rb.GetNonUnaryRuleP(key);
  if (rv) {
    //if (rv.size()>0) {
    //vector<Constit *> ch; ch.push_back(c1); ch.push_back(c2);
    Kids ch; ch.push_back(c1); ch.push_back(c2);
    for (int i=0; i<rv->size(); i++) {
      double cost = c1->tmcost + c2->tmcost + (*rv)[i]->cost;
      addConstitP((*rv)[i]->lhs,c1->start,c2->end,(*rv)[i],ch,cost);
    }
  }
}

inline void 
Decoder::tryCreateConstit(Constit *c1, Constit *c2, Constit *c3, RHS& key)
{
  const vector<Rule *> *rv = rb.GetNonUnaryRuleP(key);
  if (rv) {
    //if (rv.size()>0) {
    //vector<Constit *> ch; ch.push_back(c1); ch.push_back(c2); ch.push_back(c3);
    Kids ch; ch.push_back(c1); ch.push_back(c2); ch.push_back(c3);
    for (int i=0; i<rv->size(); i++) {
      double cost = c1->tmcost + c2->tmcost + c3->tmcost + (*rv)[i]->cost;
      addConstitP((*rv)[i]->lhs,c1->start,c3->end,(*rv)[i],ch,cost);
    }
  }
}

inline void 
Decoder::tryCreateConstit(Constit *c1, Constit *c2, Constit *c3,
			  Constit *c4, RHS& key)
{
  const vector<Rule *> *rv = rb.GetNonUnaryRuleP(key);
  if (rv) {
    //if (rv.size()>0) {
    //vector<Constit *> ch; ch.push_back(c1); ch.push_back(c2); ch.push_back(c3);
    Kids ch; ch.push_back(c1); ch.push_back(c2); ch.push_back(c3);
    ch.push_back(c4);
    for (int i=0; i<rv->size(); i++) {
      double cost = c1->tmcost + c2->tmcost + c3->tmcost + c4->tmcost + (*rv)[i]->cost;
      addConstitP((*rv)[i]->lhs,c1->start,c4->end,(*rv)[i],ch,cost);
    }
  }
}

inline void 
Decoder::tryCreateConstit(Constit *c1, Constit *c2, Constit *c3,
			  Constit *c4, Constit *c5, RHS& key)
{
  const vector<Rule *> *rv = rb.GetNonUnaryRuleP(key);
  if (rv) {
    //if (rv.size()>0) {
    //vector<Constit *> ch; ch.push_back(c1); ch.push_back(c2); ch.push_back(c3);
    Kids ch; ch.push_back(c1); ch.push_back(c2); ch.push_back(c3);
    ch.push_back(c4); ch.push_back(c5);
    for (int i=0; i<rv->size(); i++) {
      double cost = c1->tmcost + c2->tmcost + c3->tmcost
	+ c4->tmcost + c5->tmcost + (*rv)[i]->cost;
      addConstitP((*rv)[i]->lhs,c1->start,c5->end,(*rv)[i],ch,cost);
    }
  }
}

//
// Prune Constits
//

void
Decoder::replaceConstitsP(Pos start, Pos end, vector<Constit *>& newV)
{
  vector<Constit *> *oldVecPtr = constitsP[start][end];
  vector<Constit *> *newVecPtr = new vector<Constit *>;

  *newVecPtr = newV;		// copy elements
  oldVecPtr->clear();		// may not be needed
  delete oldVecPtr;
  constitsP[start][end] = newVecPtr;

}

void
Decoder::replaceConstitsC(Pos start, Pos end, vector<Constit *>& newV)
{
  vector<Constit *> *oldVecPtr = constitsC[start][end];
  vector<Constit *> *newVecPtr = new vector<Constit *>;

  *newVecPtr = newV;		// copy elements
  oldVecPtr->clear();		// may not be needed
  delete oldVecPtr;
  constitsC[start][end] = newVecPtr;
}

void
Decoder::replaceConstitsX(Pos start, Pos end, vector<Constit *>& newV)
{
  vector<Constit *> *oldVecPtr = constitsX[start][end];
  vector<Constit *> *newVecPtr = new vector<Constit *>;

  *newVecPtr = newV;		// copy elements
  oldVecPtr->clear();		// may not be needed
  delete oldVecPtr;
  constitsX[start][end] = newVecPtr;
}

void
Decoder::replaceConstitsC2(Pos start, Pos end, vector<Constit *>& newV)
{
  vector<Constit *> *oldVecPtr = constitsC2[start][end];
  vector<Constit *> *newVecPtr = new vector<Constit *>;

  *newVecPtr = newV;		// copy elements
  oldVecPtr->clear();		// may not be needed
  delete oldVecPtr;
  constitsC2[start][end] = newVecPtr;
}

void
Decoder::pruneConstits(Pos start, Pos end, ChartInfo& range)
{
  // prune constit vector

  // Neve prune a constit which is referenced by other constits, therefore
  // this must be called before applying unary/zero-length rules.

  // NYI: range must be set for later use by unary/zero rules.

  if (Params::noPrune) {
    range.minP0=range.minP1=range.minC0=range.minC1=ChartInfo::MinInit;
    return;
  }

  double min0,min1; vector<Constit *> newVec;
  
  pruneConstits(getConstitVecP(start,end),newVec,min0,min1);
  replaceConstitsP(start,end,newVec);  range.minP0=min0; range.minP1=min1;

  pruneConstits(getConstitVecC(start,end),newVec,min0,min1);
  replaceConstitsC(start,end,newVec);  range.minC0=min0; range.minC1=min1;

  pruneConstits(getConstitVecX(start,end),newVec,min0,min1);
  replaceConstitsX(start,end,newVec);

  pruneConstitsC2(getConstitVecC2(start,end),newVec);
  replaceConstitsC2(start,end,newVec);
}


void 
Decoder::pruneConstits(vector<Constit *>& cv, vector<Constit *>& newV, double& m0, double& m1)
{
  // 1. obtain min cost for constit with lmcost==0 and !=0
  // 2. discard constits less than cost+beamWidth
  // 3. hash with label & lminfo(L1/L2/R2/R1). keep only max

  const double MinInit = ChartInfo::MinInit;
  double min0=MinInit, min1=MinInit, cost;
  map<string,Constit *> hash0,hash1;
  string key(10,' ');		// 10-char len string

  // clear newV
  newV.clear();

  // obtain min cost and hash
  // Note: updateHash() will do the pruning !!

  int len=cv.size();
  for (int i=0; i<len; i++) {
    Constit *c = cv[i];
    if (c->lmcost()==0) {
      // constit with no LM cost
      cost = c->tmcost; makeConstitKey(c,key);
      if (cost < min0) min0 = cost;
      updateHash0(c,key,hash0,cost);
    } else {
      // constit with LM cost
      cost = c->tmcost + c->lmcost(); makeConstitKey(c,key);
      if (cost < min1) min1 = cost;
      updateHash1(c,key,hash1,cost);
    }
  }

  // set range info
  m0=min0; m1=min1;

  // now select constits under cost+beam
  double max0 = min0 + Params::pruneBeam;
  double max1 = min1 + Params::pruneBeam;

#if 0
  vector<Constit *> old = cv;	// copy all vector elements
  //cv.clear();
  cv = vector<Constit *>();	// free memory (see P.457 of C++ book)
#endif

  // restore from hash0
  map<string,Constit *>::const_iterator p;
  for (p=hash0.begin(); p!=hash0.end(); p++) {
    Constit *c = p->second;
    cost = c->tmcost;
    if (cost < max0) newV.push_back(c);
    else discardConstit(c);
  }
		       
  // restore from hash1
  for (p=hash1.begin(); p!=hash1.end(); p++) {
    Constit *c = p->second;
    cost = c->tmcost + c->lmcost();
    if (cost < max1) newV.push_back(c);
    else discardConstit(c);
  }
}  

void
Decoder::pruneConstitsC2(vector<Constit *>& cv, vector<Constit *>& newV)
{
  // ConstitC2 doesn't use hash pruning.

  const double MinInit = ChartInfo::MinInit;
  double min0=MinInit, min1=MinInit, cost;

  // obtain min cost
  int len=cv.size();
  for (int i=0; i<len; i++) {
    Constit *c = cv[i];
    cost = c->tmcost + c->lmcost();
    if (c->lmcost()==0) {
      if (cost < min0) min0 = cost;
    } else {
      if (cost < min1) min1 = cost;
    }
  }

  // prune over min+beam
  double max0 = min0 + Params::pruneBeam;
  double max1 = min1 + Params::pruneBeam;

  newV.clear();
  for (int i=0; i<len; i++) {
    Constit *c = cv[i];
    cost = c->tmcost + c->lmcost();
    if (c->lmcost()==0) {
      if (cost < max0) newV.push_back(c);
      else discardConstit(c);
    } else {
      if (cost < max1) newV.push_back(c);
      else discardConstit(c);
    }
  }
}

inline void
Decoder::discardConstit(Constit *c)
{
  // we may want to keep it in a file.
  //if (Params::verbose>1) { cout << "--discard: "; c->Show(); }
  c->numDealloc++;
  delete c;
}

inline void
showKey(const string& key)
{
  cout << "Key";
  for (int i=0; i<10; i++) {
    char c = key[i];
    cout << "." << int(c);
  }
  cout << " ";
}


inline void
Decoder::updateHash0(Constit *c, const string& key, map<string,Constit*>& hash, double cost)
{
  map<string,Constit *>::iterator p = hash.find(key);
  if (p==hash.end()) {		// new entry
    hash[key] = c;
  } else {
    double old = p->second->tmcost;
    if (old > cost) {
#if 0      
      if (Params::verbose>1) {
	cout << "discarding in updateHash0: "; showKey(key); cout << "\n";
	cout << "old:"; p->second->Show();
	cout << "new:"; c->Show();
      }
#endif
      discardConstit(p->second);
      p->second = c;   // replace
    } else discardConstit(c);
  }
}

inline void
Decoder::updateHash1(Constit *c, const string& key, map<string,Constit*>& hash, double cost)
{
  map<string,Constit *>::iterator p = hash.find(key);
  if (p==hash.end()) {		// new entry
    hash[key] = c;
  } else {
    double old = p->second->tmcost + p->second->lmcost();
    if (old > cost) {
#if 0
      if (Params::verbose>1) {
	cout << "discarding in updateHash1: "; showKey(key); cout << "\n";
	cout << "old:"; p->second->Show();
	cout << "new:"; c->Show();
      }
#endif
      discardConstit(p->second);
      p->second = c;   // replace
    } else discardConstit(c);
  }
}

inline void
Decoder::makeConstitKey(const Constit *c, string& key)
{
  // key must be initialized as a 10-chars string
  char c0,c1;
  sym2char(c->label,c0,c1);
  key[0]=c0; key[1]=c1;
  WordId2char(c->L1(),c0,c1);
  key[2]=c0; key[3]=c1;
  WordId2char(c->L2(),c0,c1);
  key[4]=c0; key[5]=c1;
  WordId2char(c->R2(),c0,c1);
  key[6]=c0; key[7]=c1;
  WordId2char(c->R1(),c0,c1);
  key[8]=c0; key[9]=c1;
}
  
inline void
Decoder::WordId2char(const WordId wid, char& c1, char& c2)
{
  // THIS ASSUMES WordId IS 2-BYTE LONG

  c1 = (wid & 0xff00) >> 8;
  c2 = wid & 0x00ff;
}  

inline void
Decoder::sym2char(const Symbol s, char& c1, char& c2)
{
  // THIS ASSUMES Symbol IS 2-BYTE LONG

  c1 = (s & 0xff00) >> 8;
  c2 = s & 0x00ff;
}  

//
// to be added
//

  void Decoder::appendZC(Pos start, Pos end) { }
  void Decoder::appendZP(Pos start, Pos end) { }


//
// Show Tree
//

void
Decoder::showTree(Constit *c, int indent=0)
{
  // this constit must be of V-N type

  for (int i=0; i<indent; i++) cout << " ";

  // find non-X child
  int xi; Symbol nodeX=sym("X");
  for (xi=0; xi<c->children.size(); xi++) if (c->children[xi]->label != nodeX) break;
  Constit *ch = c->children[xi];
  
  // print LHS symbol
  cout << sym(ch->label) << " ";

  // print Ridx and cost 
  if (ch->children.size()>1) 
    Reorder::ShowReorder(ch->children.size(),ch->rule->ridx);
  else cout << "\t\t";
  cout << " \t(" << c->tmcost + c->lmcost() << ") [";
  cout << "TM=" << c->tmcost << " LM=" << c->lmcost() << "] [";

  // print C-prob,R-prob,N-prob
  if (ch->isNonTerminal()) {
    // non-leaf
    cout << "+" << c->rule->cost + ch->rule->cost
	 << " r" << ch->rule->ridx << "=" << exp(-ch->rule->cost) << ",";
  } else {
    // leaf
    cout << "+" << c->rule->cost << " ";
  }
  string nstr = (c->children.front()->label==nodeX)?"left":
    ((c->children.back()->label==nodeX)?"right":"none");
  cout << nstr << "=" << exp(-(c->rule->cost)) << "]\n";
  
  // print (children or leaf) or NULL
  for (int i=0; i<c->children.size(); i++) showTree2(c->children[i],indent+1);
}

void
Decoder::showTree2(Constit *c, int indent=0)
{
  // this constit must be of V type or leaf (incl. NULL)
  
  
  if (!c->word.empty()) {
    for (int i=0; i<indent; i++) cout << " ";

    // jword may be NULL
    string jword = (c->end - c->start) ? jwords[c->start] : "NULL";

    // find CFG prob (A -> E_red)
    vector<Symbol> lbls; vector<double> cprobs;
    lbls.push_back(c->label); 
    cfg.GetLexProbs(c->word.str(),lbls,cprobs);
    double cprob = cprobs[0];

    // find ProbT (red -> akai)
    vector<string> ewords; vector<double> tprobs;
    tm.GetEWords(jword, ewords, tprobs);
    int ti=0;
    for (ti=0; ti<ewords.size(); ti++) if (ewords[ti]==c->word.str()) break;
    double tprob = tprobs[ti];

    double cost = c->tmcost + c->lmcost();
    cout << c->word.str() << " => " << jword
	 <<  "\t(" << cost << ") [+"
	 << cost << " t=" << tprob << "]\n";

  } else {
    // call in original order
    int numc=c->children.size(), ridx=c->rule->ridx;
    for (int i=0; i<numc; i++) {
      // find original nth child
      int xi;
      for (xi=0; xi<numc; xi++) if (reord(numc,ridx,xi)==i) break;
      showTree(c->children[xi],indent);
    }
  }
}

//
// ChartInfo
//

void
ChartInfo::set()
{
  // only sets from/to vars.
  // min vars must be set explicitly.

  fromP=fromC=0;
  toP=dc->getConstitVecP(start,end).size();
  toC=dc->getConstitVecC(start,end).size();
}

void
ChartInfo::update()
{
  fromP=toP; fromC=toC;
  toP=dc->getConstitVecP(start,end).size();
  toC=dc->getConstitVecC(start,end).size();
}

bool
ChartInfo::unchanged()
{
  // check if new constits are added
  return ((toP == dc->getConstitVecP(start,end).size()) &&
	  (toC == dc->getConstitVecC(start,end).size()));
}

