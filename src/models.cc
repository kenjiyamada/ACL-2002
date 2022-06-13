#include "models.h"

//
// Symbol Table
//

namespace SymbolTable {
  int maxval=0;
  map<string,Symbol> valTable;
  map<Symbol,string> strTable;
  void SplitSymbol(Symbol, char, vector<Symbol>&);
};

Symbol
sym(const string& str)
{
  Symbol s = SymbolTable::valTable[str];
  if (s==0) {
    s=++SymbolTable::maxval;
    SymbolTable::valTable[str]=s;
    SymbolTable::strTable[s]=str;
  }
  return s;
}

string &
sym(const Symbol s)
{
  return SymbolTable::strTable[s];
}

void
SymbolTable::SplitSymbol(Symbol from, char sep, vector<Symbol>& item)
{
  int start=0, len=0;
  string str; 
  str=sym(from);
  item.clear();

  for (int i=0; i<str.size(); i++) {
    if (str[i]==sep) {
      item.push_back(sym(str.substr(start,len)));
      start=i+1; len=0;
    } else len++;
  }
  item.push_back(sym(str.substr(start,len)));

}

void
SymbolTable::Dump()
{
  for (int i=1; i<=maxval; i++)
    cout << "Symbol[" << i << "]=" << sym(i) << "\n";
}

int
SymbolTable::Size()
{
  return maxval;
}


//
// Reorder
//

namespace Reorder {
  bool init_done=0;
  const int mtx1[1][1] = {{0}};
  const int mtx2[2][2] = {{0,1},{1,0}};
  int mtx3[6][3], mtx4[24][4], mtx5[120][5];
  void Init();
  int MaxReord(int);
  int Reord(int,int,int);
  void ShowReorder(int,int);
};

void
Reorder::Init()
{
  // set mtx3
  for (int i=0; i<3; i++) mtx3[0][i]=i;
  for (int j=1; j<6; j++) {
    for (int i=0; i<3; i++) mtx3[j][i]=mtx3[j-1][i];
    int *pt = &(mtx3[j][0]);
    next_permutation(pt,pt+3);
  }
  // set mtx4
  for (int i=0; i<4; i++) mtx4[0][i]=i;
  for (int j=1; j<24; j++) {
    for (int i=0; i<4; i++) mtx4[j][i]=mtx4[j-1][i];
    int *pt = &(mtx4[j][0]);
    next_permutation(pt,pt+4);
  }
  // set mtx5
  for (int i=0; i<5; i++) mtx5[0][i]=i;
  for (int j=1; j<120; j++) {
    for (int i=0; i<5; i++) mtx5[j][i]=mtx5[j-1][i];
    int *pt = &(mtx5[j][0]);
    next_permutation(pt,pt+5);
  }
 
  init_done = true;
}

int
Reorder::MaxReord(int n)
{
  switch(n) {
  case 1: return 1;
  case 2: return 2;
  case 3: return 6;
  case 4: return 24;
  case 5: return 120;
  }
}

int
Reorder::Reord(int numc, int idx, int nth)
{
  if (!init_done) Reorder::Init();

  if (numc==1) return mtx1[idx][nth];
  else if (numc==2) return mtx2[idx][nth];
  else if (numc==3) return mtx3[idx][nth];
  else if (numc==4) return mtx4[idx][nth];
  else if (numc==5) return mtx5[idx][nth];
  else {
    cout << "unexpected reorder: " << numc << "/" << idx << "/" << nth << "\n";
    error_exit();
  }
}

int
reord(int numc, int idx, int nth)
{
  // returns 'nth'-child of reorder index 'idx' for 'numc' children.
  return Reorder::Reord(numc,idx,nth);
}

void
reorder(const vector<Symbol>& in, vector<Symbol>& out, int ridx)
{
  int numc=in.size();
  out.resize(numc);
  for (int i=0; i<numc; i++)
    out[i] = in[reord(numc,ridx,i)];
}

void
Reorder::ShowReorder(int numc, int ridx)
{
  // prints "(R=1, 012->021)"
  cout << "(R=" << ridx << ", ";
  for (int i=0; i<numc; i++) cout << i;
  cout << "->";
  for (int i=0; i<numc; i++) cout << reord(numc,ridx,i);
  cout << ")";
}

//
// CList (child list of symbols) [not yet used]
//
#if 0

class ChList {
  static const int maxC = Params::MaxChildren;
  char numC;
  Symbol Children[maxC];

public:  
  ChList() : numC(0) { }
  int size() { return numC; }
  Symbol& operator[](int idx) { return Children[idx]; }

  void reorder(int ridx) {
    ChList tmp(*this);		// use default copy constructor
    for (int i=0; i<numC; i++) Children[i]=tmp[reord(numC,ridx,i)];
  }
};

#endif

//
// LM
//

LM::LM(const string &fname)
{
  if (!fname.empty()) {
    cerr << "reading " << fname << "..."; cerr.flush();
    load_lm(&ngram_model,(char *)fname.c_str());
    inUse = true;
    cerr << "done\n"; cerr.flush();
  } else {
    inUse = false;
    cerr << "no LM used\n";
  }
}

WordId
LM::GetId(const string& str)
{
  int id;

  if (!inUse) return 0;

  // upcase str
  string u_str = "";
  if (str == "<s>" || str == "</s>") u_str = str;
  else {
    for (int i=0; i<str.size(); i++) {
      char c = str[i];
      u_str += ('a' <= c && c <= 'z') ? (c + ('A' - 'a')) : c;
    }
  }

  const char *word = u_str.c_str();
  if (!sih_lookup(ng()->vocab_ht, (char *)word, &id)) id = 0;

  return id;
}

double
LM::GetLogBigram(WordId *id)
{
  double prob; int _case, verbose=2;
  //cout << "Bigram: " << id[0] << " " << id[1] << "\n"; cout.flush();
  if (!inUse) return 0;
  bo_ng_prob(1,id,ng(),verbose,&prob,&_case);
  return(-log(prob));
}

double
LM::GetLogBigram(WordId id1, WordId id2)
{
  WordId w[2];
  w[0]=id1; w[1]=id2;
  return GetLogBigram(&(w[0]));
}

double
LM::GetLogTrigram(WordId *id)
{
  double prob; int _case, verbose=2;
  //cout << "Trigram: " << id[0] << " " << id[1] << " " << id[2] << "\n"; cout.flush();
  if (!inUse) return 0;
  bo_ng_prob(2,id,ng(),verbose,&prob,&_case);
  return(-log(prob));
}

double
LM::GetLogTrigram(WordId id1, WordId id2, WordId id3)
{
  WordId w[3];
  w[0]=id1; w[1]=id2; w[2]=id3;
  return GetLogTrigram(&(w[0]));
}

//
// Word
//

int Word::maxval = 0;
map<string,WordId> Word::sidTable;
vector<string> Word::strTable;
Word Word::wordNULL;

void
Word::Show()
{
  cout << "sid=" << sid << ", vid=" << vid << "\n";
  cout << "strTable[sid]=" << strTable[sid] << "\n";
  cout << "-----------------------------\n";
}


void
Word::Clear()
{
  // clear string table
  maxval=0; 
  sidTable.clear(); 
  strTable.clear();
  strTable.push_back("DummyWord");	// dummy entry at table[0]
  
  // "NULL" at table[1]
  strTable.push_back("NULL");
  sidTable["NULL"]=1;
  maxval=1;
  wordNULL.sid=1; wordNULL.vid=0;
}

Word::Word(const string& str, LM& lm)
{
  if (strTable.size()==0) Clear();	// clear string table for the first time use

  // assign string id
  sid = sidTable[str];
  if (sid==0) {
    // new word
    sid = ++maxval;
    sidTable[str] = sid;
    strTable.push_back(str);
  }
  
  // assign LM vocab id
  vid = lm.GetId(str);
}

//
// TM
//

//class TM;			// forward declaration

TM::TM(const string& fname)
{
  ifstream in(fname.c_str());
  if (!in) error_exit("can't open TM file [" + fname + "]");
  
  for (int i=0; i<maxMode; i++) lineCnt[i]=0;

  int mode=0; 
  string line,w; vector<string> item;
  while(getline(in,line)) {
    istrstream ist(line.c_str());
    item.clear();
    while(ist>>w) item.push_back(w);
    if (item.size()>0) {
      if (item[0]=="ProbR:") mode=1;
      else if (item[0]=="ProbN:") mode=2;
      else if (item[0]=="ProbT:") mode=3;
      else if (item[0]=="Best" && item[1]=="Alignment") break;
      else {
	lineCnt[mode]++;
	string key = item[0];
	int i=1;
	if (mode==1) {
	  while(i<item.size()) {
	    tableR[key].ord.push_back(atof(item[i].c_str())); i++;
	  }
	} else if (mode==2) {
	  while(i<item.size()) {
	    tableN[key].ins[i-1] = atof(item[i].c_str()); i++;
	  }
	  // split out this node's label from key
	  Symbol keysym; vector<Symbol> splited;
	  keysym = sym(key);
	  SymbolTable::SplitSymbol(keysym,'-',splited);
	  tableN[key].keysym = keysym;
	  tableN[key].label = splited[1]; // second item, i.e. node label

	} else if (mode==3) {
	  while(i<item.size()) {
	    tableT[item[i]].rtrans[key] = atof(item[i+1].c_str()); i+=2;
	  }
	}
      }
    }
  }
}
	       
void
TM::GetProbR(const vector<Symbol>& rhs, vector<double>& prbs)
{
  // create key for tableR[] from 'rhs', and returns probs
  // for all reroders in 'prbs'.

  if (rhs.size()==1) {
    prbs.clear(); prbs.push_back(1.0);
  } else {
    string key = sym(rhs[0]);
    for (int i=1; i<rhs.size(); i++) key += ("_" + sym(rhs[i]));
    prbs = tableR[key].ord;
  }
}

bool
TM::NextTableN(Symbol& key, Symbol& label, double& none, double& right, double& left)
{
  vector<Symbol> item;

  // returns next entry of TableN
  if (iterN==tableN.end()) return false;
  else {
    key = iterN->second.keysym;
    label = iterN->second.label;

    none = iterN->second.ins[0];
    right = iterN->second.ins[1];
    left = iterN->second.ins[2];
    
    iterN++;
    return true;
  }
}

void 
TM::GetEWords(const string jword, vector<string>& ewords, vector<double>& probs)
{
  // returns all pairs of <eword,prob> given jword, 
  // s.t. tableT[jword].rtrans[eword]=prob

  EntryT &ent = tableT[jword];
  map<string,double>::const_iterator pm,begin,end;
  begin = ent.rtrans.begin(); end = ent.rtrans.end();

  ewords.clear(); probs.clear();
  for (pm=begin; pm!=end; pm++) {
    double prob = pm->second;
    if (prob > ((pm->first=="NULL") ? Params::minN : Params::minT)) {
      ewords.push_back(pm->first);
      probs.push_back(prob);
    }
  }
}


void
TM::Show()
{
  cout << "TM lines: ";
  for (int i=0; i<maxMode; i++)
    cout << "lines[" << i << "]=" << lineCnt[i] << ", ";
  cout << "\n";

  if (Params::test) {
    cout << "R[N_V_A/4]=" << tableR["N_V_A"].ord[4] << "\n";
    cout << "N[TOP-V/2]=" << tableN["TOP-V"].ins[2] << "\n";
    cout << "T[apple->juusu]=" << tableT["juusu"].rtrans["apple"] << "\n";
  }
}

//
// CFG
//

CFG::CFG(const string& fname)
{
  ifstream in(fname.c_str());
  if (!in) error_exit("can't open CFG file [" + fname + "]");

  // special flags for experiments
  bool noProbR = Params::noCfgProb;
  bool noProbL = Params::noCfgProb || Params::noCfgLexProb;

  string line,w; vector<string> item;
  while(getline(in,line)) {
    istrstream ist(line.c_str());
    item.clear();
    while(ist>>w) item.push_back(w);
    if (item.size()>0) {
      int last = item.size()-1;
      if (item[last].substr(0,2)=="E_") {
	// lexicon
	lexicon[item[last].substr(2)][sym(item[0])] =
	  ((noProbL||(item[1]=="->")) ? 1.0 : atof(item[1].c_str()));
      } else {
	// rule
	EntryC *rule = new EntryC();
	rule->lhs=sym(item[0]);
	if (item[1]=="->") {
	  rule->prob=1.0;
	  rule->numc=item.size()-2;
	  for (int i=2; i<item.size(); i++) rule->rhs[i-2]=sym(item[i]);
	} else {
	  rule->prob = noProbR ? 1.0 : atof(item[1].c_str());
	  rule->numc=item.size()-3;
	  for (int i=3; i<item.size(); i++) rule->rhs[i-3]=sym(item[i]);
	}
	rules.push_back(*rule);
      }
    }
  }
}
      
bool
CFG::Next(Symbol& lhs, vector<Symbol>& rhs, double& prob)
{
  if (iter==rules.end()) return false;
  else {
    lhs=iter->lhs;
    rhs.clear();
    for (int i=0; i<iter->numc; i++) rhs.push_back(iter->rhs[i]);
    prob=iter->prob;

    iter++;
    return true;
  }
}

void 
CFG::GetLexProbs(const string eword, vector<Symbol>& labels, vector<double>& probs)
{
  // returns all pairs of <label,prob> for eword, 
  // s.t. CFG.lexicon[eword][label]=prob

  // clear output arrays
  labels.clear(); probs.clear();

  if (eword=="NULL") {
    labels.push_back(sym("X"));
    probs.push_back(1.0);

  } else {
    map<Symbol,double>::const_iterator pm,begin,end;
    begin = lexicon[eword].begin(); end = lexicon[eword].end();
    
    for (pm=begin; pm!=end; pm++) {
      labels.push_back(pm->first);
      probs.push_back(pm->second);
    }
  }
}

void
CFG::Show()
{
  cout << "CFG size = " << rules.size() << "\n";
  EntryC *rule = &(rules[rules.size()-2]);
  cout << sym(rule->lhs) << " " << rule->prob << " -> ";
  for (int i=0; i<rule->numc; i++) cout << sym(rule->rhs[i]) << " ";
  cout << "\n";

#if 0  
  if (Params::verbose) {
    cout << "Lexicon:\n";
    typedef map<string,map<Symbol,double> >::const_iterator Iter1;
    typedef map<Symbol,double>::const_iterator Iter2;
    for (Iter1 p1=lexicon.begin(); p1!=lexicon.end(); p1++) {
      const map<Symbol,double>& tmap = p1->second;
      for (Iter2 p2=tmap.begin(); p2!=tmap.end(); p2++) {
	cout << sym(p2->first) << " " << p2->second << " -> " << p1->first << "\n";
      }
    }
  }
#endif
}

