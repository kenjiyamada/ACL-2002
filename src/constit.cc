#include "constit.h"

//
// LM Info
//

bool
LMInfo::init_done = false;

bool
LMInfo::noLM = true;

WordId
LMInfo::Empty = 1;

WordId
LMInfo::BOS = 0;

WordId
LMInfo::EOS = 0;

void
LMInfo::classInit(LM& lm)
{
  init_done = true;
  if (lm.InUse()) {
    noLM = false;
    BOS = Empty = lm.GetId("<s>");
    EOS = lm.GetId("</s>");
    cout << "Empty==" << Empty << "\n";
  }
}

// Encoding LMInfo of less than two words (*==Empty)
//
//         L1 L2 R2 R1
// len=0    *  0
// len=1    *  1     w

inline int
LMInfo::len()
{
  // returns 0, 1 or 2 (means 2orMore)
  if (L1 != Empty) return 2;
  else return L2;
}

inline WordId
LMInfo::getSingletonW()
{
  return R1;
}

inline void
LMInfo::setZeroLength()
{
  L1 = Empty; L2 = 0;
}

inline void
LMInfo::setSingletonW(WordId w)
{
  L1 = Empty; L2 = 1;
  R1 = w;
}

void
LMInfo::setTerminal(Constit *c)
{
  // set LMInfo for terminal constit
  // assuming constit length == 1

  if (c->word.isNULL()) setZeroLength();
  else {
    // if LM is not used, use string id for L1/L2/R2/R1.
    WordId wid = noLM ? c->word.id() : c->word.LMid();
    setSingletonW(wid);
  }
}

void
LMInfo::set(LM& lm, Kids& c0, int ridx)
{
  // This is called only on creating a non-terminal constit.

  // L1 and L2 are leftmost two words in the constit, and trigrams for L1 and L2
  // are not yet obtained.
  // R2 and R1 are rightmost two words. Those are just for obtaining trigram for
  // succeeding words.

  // If no LM is used, string ID is used instead. So we don't return here.
  // This is to avoid pruning constits without looking the terminal words difference.
  //if (noLM) return;   // OLD CODE

  // short-cut: unary-rule application keeps LM the same.
  if (c0.size()==1) { *this = c0[0]->lminfo; return; }


  // restore original order
  //vector<Constit *>c; restoreOrder(c0,c,ridx);
  Kids c; restoreOrder(c0,c,ridx);

  WordId w1=Empty, w2=Empty;
  double lmcost=0.0;

  // shift boundary words into (w1,w2)
  for (int i=0; i<c.size(); i++) {
    lmcost += c[i]->lmcost0();

    int len = c[i]->lmlen();
    if (len==0) { 
      // do nothing
    } else if (len==1) {
      if (w1!=Empty) lmcost += lm.GetLogTrigram(w1,w2,c[i]->lmSingletonW());
      w1=w2; w2=c[i]->lmSingletonW();
    } else { // len>=2
      // how about bigram??
      if (w1!=Empty) lmcost += lm.GetLogTrigram(w1,w2,c[i]->L1());
      if (w2!=Empty) lmcost += lm.GetLogTrigram(w2,c[i]->L1(),c[i]->L2());
      w1=c[i]->R2(); w2=c[i]->R1();
    }
  }

  // set lmcost
  this->cost = lmcost;

  // set L1,L2
  L1=L2=Empty;			// init
  for (int i=0; i<c.size(); i++) {
    int len = c[i]->lmlen();
    if (len >= 2) {
      if (L1==Empty) {
	L1 = c[i]->L1(); L2 = c[i]->L2();
      } else {
	L2 = c[i]->L1();
      }
      break;
    } else if (len == 1) {
      if (L1==Empty) L1 = c[i]->lmSingletonW();
      else {
	L2 = c[i]->lmSingletonW(); break;
      }
    } // ignore len==0
  }

  // set R2,R1 and len
  R2=R1=Empty;			// init
  int totlen=0;
  for (int i=c.size()-1; i>=0; i--) {
    int len = c[i]->lmlen();
    totlen += len;
    if (len >=2 ) {
      if (R1==Empty) {
	R2 = c[i]->R2(); R1 = c[i]->R1();
      } else {
	R2 = c[i]->R1();
      }
      break;
    } else if (len == 1) {
      if (R1==Empty) R1 = c[i]->lmSingletonW();
      else {
	R2 = c[i]->lmSingletonW(); break;
      }
    } // ignore len==0
  }

  // set totlen
  if (totlen==0) setZeroLength();
  else if (totlen==1) setSingletonW(R1);

}

void
LMInfo::addSentBoundaryLM(LM& lm)
{
  if (!noLM && len() >= 2) {
    // if bcost is already set, do nothing.
    if (bcost>0) return;
    bcost = lm.GetLogBigram(BOS,L1);
    bcost += lm.GetLogTrigram(BOS,L1,L2);
    bcost += lm.GetLogTrigram(R2,R1,EOS);
    cost += bcost;
  }
}

//
// Constit
//

int
Constit::numAlloc = 0;

int
Constit::numDealloc = 0;

void
Constit::checkAlloc()
{
  if (!Params::verbose) return;
  numAlloc++;
  int numConstits = numAlloc-numDealloc;
  //if (numConstits % 100 == 0) showAlloc();

}

void
Constit::showAlloc()
{
  int numConstits = numAlloc-numDealloc;
  cout << "#Constits=" << numConstits
       << "(Alloc=" << numAlloc << ",Dealloc=" << numDealloc << ")"
       << ", MemSize=" << MemSize()
       << ", CPUTime=" << UTime() << "\n";
  cout.flush();
}  


void 
Constit::Show(int indent=0)
{
  for (int i=0; i<indent; i++) cout << " ";

  cout << "[" << sym(label) << ":" << int(start) << "," << int(end) << "] ";
  if (Params::verbose>2) cout << "(" << this << ") ";
  if (isTerminal()) cout << word.str() << " ";
  ShowCost(tmcost,lmcost());

  if (Params::verbose>2) {
    if (isNonTerminal())
      for (int j=0; j<children.size(); j++) children[j]->Show(indent+1);
  }
  cout.flush();
}

void
Constit::ShowOrg(int indent=0)
{
  for (int i=0; i<indent; i++) cout << " ";
  cout << "[" << sym(label) << ":" << int(start) << "," << int(end) << "]";
  if (Params::verbose>2) cout << " (" << this << ") ";
  if (isTerminal()) { cout << " " << word.str(); ShowCost(tmcost,lmcost()); }

  if (isNonTerminal()) {
    int numc=children.size(), ridx=rule->ridx;
    if (numc>1) cout << " R=" << ridx;
    ShowCost(tmcost,lmcost());

    for (int i=0; i<numc; i++) {
      // find original nth child
      int xi;
      for (xi=0; xi<numc; xi++) if (reord(numc,ridx,xi)==i) break;
      children[xi]->ShowOrg(indent+1);
    }
    cout.flush();
  }  
}

void
Constit::GetOrgEWords(string& esent)
{
  // fill ewords vector with original order English words.
  // esent must be empty before calling

  if (isTerminal()) {
    if (Params::verbose || word.str() != "NULL") {
      if (esent.size()!=0) esent += " ";
      esent += word.str();
    }
  } else {
    int numc=children.size(), ridx=rule->ridx;
    for (int i=0; i<numc; i++) {
      // find original nth child
      int xi;
      for (xi=0; xi<numc; xi++) if (reord(numc,ridx,xi)==i) break;
      children[xi]->GetOrgEWords(esent);
    }
  }
}

//
// misc functions
//

void
restoreOrder(Kids& in, Kids& out, int ridx)
{
  int numc=in.size();

  if (ridx==0) {
    out=in; return;
  } else {
    out.clear();
    for (int i=0; i<numc; i++) {
      int xi;
      for (xi=0; xi<numc; xi++) if (reord(numc,ridx,xi)==i) break;
      out.push_back(in[xi]);
    }
  }
}

bool
ConstitLt(const Constit* x, const Constit* y)
{
  return (x->Cost() < y->Cost());
}

