#ifndef CONSTIT_H
#define CONSTIT_H

#include "models.h"
#include "rules.h"
#include "misc.h"

//
// LM Info
//

class Constit;			// forward declaration
class Kids;

class LMInfo {
  friend class Constit;

  // class common data
  static bool init_done;
  static bool noLM;
  static WordId Empty;
  static WordId BOS,EOS;

  // instance specific data
  double cost;
  double bcost;			// sentence boundary cost (included in cost)
  WordId L1,L2,R2,R1;

  LMInfo() { L1=L2=R1=R2=Empty; cost=bcost=0; }

public:
  static void classInit(LM& lm);

private:
  //void set(LM& lm, vector<Constit *>& children, int ridx);
  void set(LM& lm, Kids& children, int ridx);
  void setTerminal(Constit *c);
  void addSentBoundaryLM(LM& lm);

  inline int len();
  inline WordId getSingletonW();
  inline void setSingletonW(WordId w);
  inline void setZeroLength();

};

//
// Constit
//

typedef char Pos;

class Decoder;			// forward declaration
class Constit;

class Kids {
  static const int maxc = Params::MaxChildren;
  char numc;
 public:
  Constit *children[maxc];

  Kids() { numc=0; }
  Kids(vector<Constit *>& cv) {
    numc=cv.size(); for (int i=0; i<numc; i++) children[i]=cv[i];
  }
  Kids(const Kids& x) {
    numc=x.size(); for (int i=0; i<numc; i++) children[i]=x.children[i];
  }
  
  int size() const { return numc; }
  void clear() { numc=0; }
  Constit *operator[](int i) { return children[i]; }
  void push_back(Constit *c) { children[numc++] = c; }
  Constit *front() { return children[0]; }
  Constit *back() { return children[numc-1]; }
};

class Constit {
  friend class LMInfo;
  friend class Decoder;

  // for debug
  static int numAlloc;
  static int numDealloc;
  void checkAlloc();
  static void showAlloc();

  Symbol label;
  Pos start,end;		// needed by Show() only
  Rule *rule;
  //vector<Constit *> children;	// for non-terminal
  Kids children;	// for non-terminal
  Word word;			// for terminal
  double tmcost;
  LMInfo lminfo;
  
  // constructor for terminal
  Constit(Symbol l, Pos s, Pos e, Word w, double c)
    : label(l), start(s), end(e), rule(0), word(w), tmcost(c)
  { lminfo.setTerminal(this); }

  // constructor for non-terminal
  Constit(Symbol l, Pos s, Pos e, Rule *r, Kids& ch, double c, LM& lm)
    : label(l), start(s), end(e), rule(r), children(ch), word(), tmcost(c), lminfo()
  { checkAlloc(); lminfo.set(lm,ch,r->ridx); }

  // constructor for non-terminal (C2-constit)
  Constit(Symbol l, Pos s, Pos e, Rule *r, Kids& ch, double tc, double lc)
    : label(l), start(s), end(e), rule(r), children(ch), word(), tmcost(tc)
  { checkAlloc(); setC2lmcost(lc); }

  // for constructor for expanding C2-constit,
  // use default copy constructor

  bool isTerminal() { return (rule==0); }
  bool isNonTerminal() { return !isTerminal(); }

  // LMInfo functions
  double lmcost() const { return lminfo.cost; }
  double lmcost0() const { return lminfo.cost - lminfo.bcost; }
  void setC2lmcost(double c) { lminfo.cost = c; }
  void addSentBoundaryLM(LM& lm) { lminfo.addSentBoundaryLM(lm); }
  int lmlen() { return lminfo.len(); }
  WordId lmSingletonW() { return lminfo.getSingletonW(); }
  WordId L1() const { return lminfo.L1; }
  WordId L2() const { return lminfo.L2; }
  WordId R2() const { return lminfo.R2; }
  WordId R1() const { return lminfo.R1; }

public:
  double Cost() const { return tmcost+lmcost(); }
  void Show(int indent=0);
  void ShowOrg(int indent=0);
  void GetOrgEWords(string& esent);
};


//void restoreOrder(const vector<Constit *>& in, vector<Constit *>& out, int ridx);
void restoreOrder(Kids& in, Kids& out, int ridx);
bool ConstitLt(const Constit* x, const Constit* y);


#endif // CONSTIT_H
