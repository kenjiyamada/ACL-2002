#ifndef RULES_H
#define RULES_H

#include "models.h"

class RuleBox;			// forward declaration
class Decoder;
class Constit;


class RHS {
  static const int maxc = Params::MaxChildren;
  char numc;
 public:
  Symbol children[maxc];

  inline RHS() { numc=0; }
  inline RHS(vector<Symbol>& cv) {
    numc=cv.size(); for (int i=0; i<numc; i++) children[i]=cv[i];
  }
  inline RHS(const RHS& x) {
    numc=x.size(); for (int i=0; i<numc; i++) children[i]=x.children[i];
  }

  int size() const { return numc; }
  bool operator<(const RHS& x) const {
    if (numc < x.size()) return true;
    if (numc > x.size()) return false;
    for (int i=0; i<numc; i++)
      if (children[i] < x.children[i]) return true;
      else if (children[i] > x.children[i]) return false;
    return false;
  }
  void push_back(Symbol s) { children[numc++]=s; }
};

class Rule {
  friend class RuleBox;
  friend class Constit;
  friend class Decoder;
  Symbol lhs;
  vector<Symbol> rhs;
  double cost;
  int ridx;			// reorder index

  Rule(Symbol lhs, vector<Symbol> rhs, double cost, int ridx=0)
    : lhs(lhs), rhs(rhs), cost(cost), ridx(ridx) { }    
};


class RuleBox {
  // note: nonUnaryRulesP[rhs] has unique lhs, but has multiple ridx.

  static const int maxSymbols = Params::MaxSymbols;

  // P-rules
  vector<Rule *> *nonUnaryRulesP2[maxSymbols][maxSymbols];
  map<RHS,vector<Rule *> > nonUnaryRulesP;          // VB -> VB-VB VB-NN VB-VB
  map<Symbol,Rule *> unaryRulesP;                   // TOP -> TOP-VB
  // C-rules
  map<Symbol,vector<Rule *> > rulesPX;              // VB-NN -> NN X
  map<Symbol,vector<Rule *> > rulesXP;              // VB-NN -> X NN
  map<Symbol,vector<Rule *> > unaryRulesC;          // VB-NN -> NN

  // list to hold each rules
  vector<Rule *> allRules;

public:
  RuleBox() { initP2(); }
  void set(TM& tm, CFG& cfg);
  void clear();
  bool empty() { return (allRules.size()==0); }

  inline const vector<Rule *> *GetNonUnaryRulesP2(Symbol s1, Symbol s2) const
    { return nonUnaryRulesP2[s1][s2]; }

  inline const vector<Rule *> *GetNonUnaryRuleP(RHS& key) const {
    map<RHS,vector<Rule*> >::const_iterator p = nonUnaryRulesP.find(key);
    if (p==nonUnaryRulesP.end()) return 0;
    else return &(p->second);
  }
  Rule *GetUnaryRuleP(Symbol rhs) { return unaryRulesP[rhs]; }
  vector<Rule *>& GetUnaryRulesC(Symbol rhs) { return unaryRulesC[rhs]; }
  vector<Rule *>& GetRulesPX(Symbol rhs) { return rulesPX[rhs]; }
  vector<Rule *>& GetRulesXP(Symbol rhs) { return rulesXP[rhs]; }


private:
  void initP2();
  Rule *newRule(Symbol, vector<Symbol>, double, int=0);
  void setFromProbR(TM& tm, CFG& cfg);
  void setFromProbN(TM& tm);
  void addNonUnaryRuleP2(Rule *);


  // misc functions
  //void makeRuleKey(const vector<Symbol>& rhs, string& key);

};

#endif // RULES_H
