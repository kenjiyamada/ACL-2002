#include "decode3.h"

#ifdef CHEAT_INFO

static const int maxSentLen = Params::MaxSentLen;
bool useCheatInfo = false;

class CheatInfo {
public:
  Symbol label;
  string type;
  string subtype;
  double tmcost;
  double lmcost;
  string esent;
};

vector<CheatInfo> cheatData[maxSentLen][maxSentLen];


bool
findConstit(Constit *c, vector<Constit *>&vec)
{
  for (int i=0; i<vec.size(); i++)
    if (vec[i]==c) return true;
  return false;
}

void
Decoder::showConstitType(Constit *c)
{
  Pos start=c->start, end=c->end;
  string type;

  if (findConstit(c,getConstitVecP(start,end)))
    type = "P";
  else if (findConstit(c,getConstitVecC(start,end)))
    type = "C";
  else if (findConstit(c,getConstitVecC2(start,end)))
    type = "C2";
  else
    type = "??";

  string subtype = (c->lmcost()==0) ? "0" : "1";

  cout << type << " " << subtype;
}

bool
Decoder::matchC2(Constit *c0, int nc, Constit *c2)
{
  if (c0->label != c2->label) return false;
  if (c0->rule != c2->rule) return false;
  if (c2->children.size() != nc) return false;
  for (int i=0; i<nc; i++)
    if (c0->children[i] != c2->children[i]) return false;
  return true;
}


Constit *
Decoder::findConstitC2(Constit *c, int nc, vector<Constit*>& vec)
{
  Pos end2 = c->children[nc-1]->end;
  Constit *c2 = 0;
  for (int i=0; i<vec.size(); i++) {
    c2 = vec[i];
    if (matchC2(c,nc,c2)) return c2;
  }
  return 0;
}

void
Decoder::showCheatC2(Constit *c)
{
  cout << int(c->start) << " " << int(c->end) << " " << sym(c->label) << " ";
  showConstitType(c);
  cout << " " << c->tmcost;
  cout << " " << c->lmcost();
  string esent; c->children[0]->GetOrgEWords(esent);
  cout << " " << esent;
  int numc=c->children.size();
  for (int i=1; i<numc; i++) {
    esent=""; c->children[i]->GetOrgEWords(esent);
    cout << " + " << esent;
  }
  cout << "\n";
}

void
Decoder::showCheat(Constit *c)
{
  // print out cheating info (non-terminal only)
  if (c->isNonTerminal()) {
    cout << int(c->start) << " " << int(c->end) << " " << sym(c->label) << " ";
    showConstitType(c);
    cout << " " << c->tmcost;
    cout << " " << c->lmcost();
    string esent; c->GetOrgEWords(esent);
    cout << " " << esent;
    cout << "\n";

    // find & show C2 constit
    int numc = c->children.size();
    if (numc>2) {
      for (int i=1; i<numc-1; i++) {
	int c2end = c->children[i]->end;
	vector<Constit *>& vec = getConstitVecC2(c->start,c2end);
	Constit *c2 = findConstitC2(c,i+1,vec);
	if (c2==0) error_exit("C2 not found!!");
	if (c2) showCheatC2(c2);
      }
    }
	  
    // recursive call for children
    for (int i=0; i<c->children.size(); i++) showCheat(c->children[i]);
  }
}

void
Decoder::readCheatInfo()
{
  string fname = Params::cheatFile;

  if (fname.size()>0) {
    useCheatInfo=true;
    ifstream in(fname.c_str());
    if (!in) error_exit("can't open Cheat File [" + fname + "]");

    cout << "reading CheatInfo..."; cout.flush();
    bool on=false;
    string line,w; vector<string> item;
    while(getline(in,line)) {
      istrstream ist(line.c_str());
      item.clear();
      while(ist>>w) item.push_back(w);
      if (item.size()>0) {
	if (item[0]=="CheatInfo:") on=true;
	else if (on) {
	  Pos start,end; 
	  CheatInfo ci;

	  start = atoi(item[0].c_str());
	  end = atoi(item[1].c_str());
	  ci.label = sym(item[2]);
	  ci.type = item[3];
	  ci.subtype = item[4];
	  ci.tmcost = atof(item[5].c_str());
	  ci.lmcost = atof(item[6].c_str());
	  ci.esent = item[7];
	  for (int i=8; i<item.size(); i++) ci.esent += (" " + item[i]);

	  cheatData[start][end].push_back(ci);
	}
      }
    }
    cout << "done\n"; cout.flush();
  }
}

bool
Decoder::pruneCheatInfo(Symbol label, Pos start, Pos end)
{
  if (!useCheatInfo || !Params::cheatPrune) return true;

  // This function should be called just before creating a "NonTerminal" constit.

  vector<CheatInfo>& vec = cheatData[start][end];
  bool found=false;
  for (int i=0; i<vec.size(); i++) if (vec[i].label==label) found=true;
  return found;
}

void
Decoder::inspectCheatInfo(Pos start, Pos end)
{
  if (!useCheatInfo) return;

  // Display constits if they are found in CheatInfo.
  vector<CheatInfo>& cheatVec = cheatData[start][end];
  for (int i=0; i<cheatVec.size(); i++) {
    CheatInfo ci = cheatVec[i];
    double cost = ci.tmcost + ci.lmcost;
    cout << "CheatInfo: " << int(start) << " " << int(end)
	 << " (" << cost << ") " << sym(ci.label)
	 << " (" << ci.type << "-" << ci.subtype << ") ["
	 << ci.esent << "] ";
    cout << "[TM=" << ci.tmcost << " LM=" << ci.lmcost << "] ";
    if (ci.type=="P")
      inspectCheatInfo(start,end,(ci.subtype=="0"),cost,getConstitVecP(start,end));
    if (ci.type=="C")
      inspectCheatInfo(start,end,(ci.subtype=="0"),cost,getConstitVecC(start,end));
    if (ci.type=="C2")
      inspectCheatInfoC2(start,end,(ci.subtype=="0"),cost,getConstitVecC2(start,end));
    cout << "\n";
  }
  cout.flush();
}

void
Decoder::inspectCheatInfo(Pos start, Pos end, bool find0, double cost,
			  vector<Constit *>& cv)
{
  vector<Constit *> sortV = cv;	// copy
  sort(sortV.begin(),sortV.end(),ConstitLt);

  // find best score constit (within the subtype)
  bool found=false;
  Constit *c;
  for (int i=0; i<sortV.size(); i++) {
    c = sortV[i];
    bool found0 = (c->lmcost()==0);
    if ((find0 && found0) || (!find0 && !found0)) {
      found=true; break;
    }
  }
  if (found) {
    double mincost = c->tmcost + c->lmcost();
    double diff = cost - mincost;
    if (diff>0) cout << "+";
    cout << diff << "\n";

    // print the best (or all) constit in the vector
    if (Params::cheatVecAll) {
      // show all
      for (int i=0; i<sortV.size(); i++) {
	c = sortV[i];
	bool found0 = (c->lmcost()==0);
	if ((find0 && found0) || (!find0 && !found0)) inspectCheatShow(c);
      }
    } else {       
      // show only the best
      inspectCheatShow(c);
    }
    
  } else cout << "\n";
}

void
Decoder::inspectCheatInfoC2(Pos start, Pos end, bool find0, double cost,
			    vector<Constit*>& cv)
{
  if (cv.size()==0) return;

  vector<Constit *> sortV = cv;	// copy
  sort(sortV.begin(),sortV.end(),ConstitLt);

  // find best score constit (within the subtype)
  bool found=false;
  Constit *c;
  for (int i=0; i<sortV.size(); i++) {
    c = sortV[i];
    bool found0 = (c->lmcost()==0);
    if ((find0 && found0) || (!find0 && !found0)) {
      found=true; break;
    }
  }
  if (found) {
    double mincost = c->tmcost + c->lmcost();
    double diff = cost - mincost;
    if (diff>0) cout << "+";
    cout << diff << "\n";
    
    // print the best (or all) constit in the vector
    if (Params::cheatVecAll) {
      // show all
      for (int i=0; i<sortV.size(); i++) {
	c = sortV[i];
	bool found0 = (c->lmcost()==0);
	if ((find0 && found0) || (!find0 && !found0)) inspectCheatShow(c,true);
      }
    } else {       
      // show only the best
      inspectCheatShow(c,true);
    }
    
  } else cout << "\n";
}

void
Decoder::inspectCheatShow(Constit *c, bool isC2=false)
{
  double cost = c->tmcost +c->lmcost();
  cout << int(c->start) << " " << int(c->end) << " "
       << "(" << cost << ") "
       << sym(c->label) << " [";

  // print esent
  if (isC2) {
    // esent must be obtained per each child (since C2 is not complete)
    for (int i=0; i<c->children.size(); i++) {
      Constit *cc = c->children[i];
      string esent; cc->GetOrgEWords(esent);
      if (i!=0) cout << " + ";
      cout << esent;
    }
    cout << "]";
  } else {
    string esent; c->GetOrgEWords(esent);
    cout << esent << "]";
  }

  // show costs
  cout << " [TM=" << c->tmcost << " LM=" << c->lmcost() << "]\n";

  // show internal structure, if -v is set
  if (Params::verbose) {
    if (isC2) {
      // it cannot call c->ShowOrg(), since this constit is not complete
      cout << "RuleCost=" << c->rule->cost << "\n";
      for (int i=0; i<c->children.size(); i++) {
	Constit *cc = c->children[i];
	cc->ShowOrg();
      }
    } else {
      c->ShowOrg();
    }
  }
}

#endif // CHEAT_INFO
