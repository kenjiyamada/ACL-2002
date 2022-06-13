#ifndef DECODE3_H
#define DECODE3_H

#include "misc.h"
#include "models.h"
#include "rules.h"
#include "constit.h"


// debugging info
#define CHEAT_INFO

// P-rule:  VB -> VB-NN VB-NN VB-VB
// C-rule:  VB-VB -> VB X

// P-constit: VB, X, etc
// C-constit: VB-NN, etc

// Rule Application:
//     C-constit + P-rule => P-constit
//     P-constit + C-rule => C-constit

// L-constit: part of P-constits (includes zero-length constit)
// X-constit: same as L-constits, but stored separately
//        (never zero-length, it can be >1 in future, only used by PX/XP rules)

//
// Decoder class
//

class ChartInfo;		// forward declaration

class Decoder {
  friend class ChartInfo;
  static const int maxUnary = Params::MaxUnary;
  static const int maxXConstitLen = Params::MaxXConstitLen;
  static const int maxSentLen = Params::MaxSentLen;

  LM& lm;
  TM& tm;
  CFG& cfg;
  vector<string> jwords;	// input words.

  // rule tables
  RuleBox rb;

  // chart data ([0,0] is used for ZeroLength constits)
  vector<Constit *> *constitsP[maxSentLen][maxSentLen];
  vector<Constit *> *constitsC[maxSentLen][maxSentLen];
  vector<Constit *> *constitsX[maxSentLen][maxSentLen];
  vector<Constit *> *constitsC2[maxSentLen][maxSentLen];

  // misc vars
  Symbol topSymbol;
  Symbol symX;


public:
  Decoder(LM& lm, TM& tm, CFG& cfg);
  void DecodeInputs(istream& in);

private:
  void allocConstitVecs();
  void clear();
  void resetRules() { rb.set(tm,cfg); }
  void clearChart();

  void decodeIt(vector<string>& words); // setup vars
  void mainLoop();		// main loop
  void showResults();

  // chart access functions
  inline vector<Constit *>& getConstitVecP(Pos start, Pos end)
    { return *constitsP[start][end]; }
  inline vector<Constit *>& getConstitVecC(Pos start, Pos end)
    { return *constitsC[start][end]; }
  inline vector<Constit *>& getConstitVecX(Pos start, Pos end)
    { return *constitsX[start][end]; }
  inline vector<Constit *>& getConstitVecC2(Pos start, Pos end)
    { return *constitsC2[start][end]; }

  void replaceConstitsP(Pos start, Pos end, vector<Constit *>& newV);
  void replaceConstitsC(Pos start, Pos end, vector<Constit *>& newV);
  void replaceConstitsX(Pos start, Pos end, vector<Constit *>& newV);
  void replaceConstitsC2(Pos start, Pos end, vector<Constit *>& newV);


  // create/add new constit

  void addLexConstit(Symbol label, Pos start, Pos end, string& eword, double cost) {
    Constit *c = new Constit(label,start,end,Word(eword,lm),cost);
    (label==symX) ? getConstitVecX(start,end).push_back(c) :
      getConstitVecP(start,end).push_back(c);     
  }

  void addConstitP(Symbol label, Pos start, Pos end, Rule *r,
		   Kids& ch, double cost) {
#ifdef CHEAT_INFO
    if (!pruneCheatInfo(label,start,end)) return;
#endif
    if (Params::debug) { cout << "\n>>>adding new Constit\n"; Constit::showAlloc(); }
    Constit *c = new Constit(label,start,end,r,ch,cost,lm);
    if (Params::debug) { cout << "<<<added  new Constit\n"; Constit::showAlloc(); }
    getConstitVecP(start,end).push_back(c);     

    // sentence boundary Trigram
    if (start==0 && end==sentLen()) c->addSentBoundaryLM(lm);
  }

  void addConstitC(Symbol label, Pos start, Pos end, Rule *r,
		   Kids& ch, double cost) {
#ifdef CHEAT_INFO
    if (!pruneCheatInfo(label,start,end)) return;
#endif
    Constit *c = new Constit(label,start,end,r,ch,cost,lm);
    getConstitVecC(start,end).push_back(c);
  }

  void addConstitC2(Symbol label, Pos start, Pos end, Rule *r,
		   Kids& ch, double tmcost, double lmcost) {
    Constit *c = new Constit(label,start,end,r,ch,tmcost,lmcost);
    getConstitVecC2(start,end).push_back(c);
  }

  void expandConstitC2(Constit *c2, Constit *ch, Pos newend) {
    Constit *c = new Constit(*c2); // default copy constructor
    c->children.push_back(ch);
    c->tmcost += ch->tmcost;
    c->setC2lmcost(c->lmcost() + ch->lmcost());
    c->end = newend;
    getConstitVecC2(ch->start,newend).push_back(c);
  }

  // subroutines
  void initBaseLexConstits();
  void initZeroLengthConstits();
  void createLexConstits(const string&,int,int);
  void initBaseLM();
  void applyNonUnaryRules(Pos,Pos);
  void applyUnaryRules(Pos,Pos,ChartInfo&);
  void addZeroLengthConstits(Pos,Pos,ChartInfo&);
  void tryAllPartitions(Pos,Pos);
  inline void tryCreateConstit(Constit*,Constit*,RHS&);
  inline void tryCreateConstit(Constit*,Constit*,Constit*,RHS&);
  inline void tryCreateConstit(Constit*,Constit*,Constit*,Constit*,RHS&);
  inline void tryCreateConstit(Constit*,Constit*,Constit*,Constit*,Constit*,RHS&);
  void combineXP(Pos,Pos,Pos);
  void combinePX(Pos,Pos,Pos);
  void combineCC(Pos,Pos,Pos);
  void combineC2C(Pos,Pos,Pos);
  void applyUP(Pos,Pos,ChartInfo&);
  void applyUC(Pos,Pos,ChartInfo&);
  inline bool unaryWithinBeamP(Constit*,double,ChartInfo&);
  inline bool unaryWithinBeamC(Constit*,double,ChartInfo&);
  void appendZC(Pos,Pos);
  void appendZP(Pos,Pos);

  // prune constits
  void pruneConstits(Pos,Pos,ChartInfo&);
  void pruneConstits(vector<Constit *>&,vector<Constit *>&,double&,double&);
  void pruneConstitsC2(vector<Constit *>&,vector<Constit *>&);
  inline void discardConstit(Constit *);
  inline void updateHash0(Constit *,const string&,map<string,Constit*>&,double);
  inline void updateHash1(Constit *,const string&,map<string,Constit*>&,double);
  //inline void updateHash(const string&,map<string,double>&,double);
  //inline bool aboveHash(const string&,map<string,double>&,double);
  inline void makeConstitKey(const Constit *,string&);
  inline void WordId2char(const WordId,char&,char&);
  inline void sym2char(const Symbol,char&,char&);

#ifdef CHEAT_INFO
  void showConstitType(Constit *c);
  bool matchC2(Constit*,int,Constit*);
  Constit *findConstitC2(Constit*,int,vector<Constit*>&);
  void showCheatC2(Constit *);
  void showCheat(Constit *c);
  void readCheatInfo();
  bool pruneCheatInfo(Symbol,Pos,Pos);
  void inspectCheatInfo(Pos,Pos);
  void inspectCheatInfo(Pos,Pos,bool,double,vector<Constit *>&);
  void inspectCheatInfoC2(Pos,Pos,bool,double,vector<Constit *>&);
  void inspectCheatShow(Constit *,bool=false);
#endif

  // misc functions
  int sentLen() { return jwords.size(); }
  bool rulesEmpty() { return rb.empty(); }

  void showTree(Constit *c, int indent=0);
  void showTree2(Constit *c, int indent=0);
  void showChartLen(Pos,Pos,string);
};

//
// Misc supporting classes
//

class ChartInfo {
  Decoder *dc;
  Pos start,end;
public:
  static const double MinInit = 9999;
  int fromP, toP;
  int fromC, toC;
  double minP0,minP1,minC0,minC1;

  ChartInfo(Decoder *dc, Pos start, Pos end) : dc(dc), start(start), end(end) { }
  void set();
  void update();
  bool unchanged();
};

#endif // DECODE3_H
