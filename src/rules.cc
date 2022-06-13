#include "rules.h"

//
// Rule
//

//
// RuleBox
//

void
RuleBox::clear()
{

  nonUnaryRulesP.clear();
  unaryRulesP.clear();
  rulesPX.clear();
  rulesXP.clear();
  unaryRulesC.clear();

  for (int i=0; i<allRules.size(); i++) delete allRules[i];
  allRules.clear();

  // clear nonUnaryRulesP2[][]
  for (int i=0; i<maxSymbols; i++)
    for (int j=0; j<maxSymbols; j++) {
      vector<Rule *> *r = nonUnaryRulesP2[i][j];
      delete r;			// dealloc vector<>
      nonUnaryRulesP2[i][j] = 0;
    }
}

void
RuleBox::set(TM& tm, CFG& cfg)
{
  // remove old stuff (if any)
  clear();

  // create rules from CFG and TM
  setFromProbR(tm,cfg);
  setFromProbN(tm);
}

//
// RuleBox private functions
//

void
RuleBox::initP2()
{
  // alloc nonUnaryRulesP2[][]

  if (SymbolTable::Size() >= maxSymbols) error_exit("too many symbols");

  for (int i=0; i<maxSymbols; i++)
    for (int j=0; j<maxSymbols; j++)
      nonUnaryRulesP2[i][j] = 0;
}


Rule *
RuleBox::newRule(Symbol lhs, vector<Symbol> rhs, double cost, int ridx=0)
{
  Rule *rule = new Rule(lhs,rhs,cost,ridx);
  allRules.push_back(rule);
  return rule;
}

void
RuleBox::setFromProbR(TM& tm, CFG& cfg)
{
  // create rules from CFG and TM(probR)
  Symbol lhs; vector<Symbol> rhs,rrhs;
  double cprob,cost;
  vector<double> prbs;	// holds probs for all reorders
  Rule *rule; //string key;

  cfg.Rewind();

  // For each CFG rule
  while(cfg.Next(lhs,rhs,cprob)) {
    // obtain R-prob
    tm.GetProbR(rhs,prbs);

    // prefix children with parent label (ex. V->N A => V->V-N V-A)
    for (int i=0; i<rhs.size(); i++) rhs[i]=sym(sym(lhs)+"-"+sym(rhs[i]));

    // add the rule if prob is above threshold
    for (int ri=0; ri<prbs.size(); ri++) {
      if (prbs[ri]>=Params::minR) {
	reorder(rhs,rrhs,ri);	      // rhs -> rrhs (reorder)
	cost = -log(cprob*prbs[ri]);  // P(cfg)*P(reord)
	rule = newRule(lhs,rrhs,cost,ri);

	if (rhs.size()==1) unaryRulesP[rhs[0]]=rule;
	else {
#if 1
	  // ===This section will be obsolete if RuleP2 is in full effect
	  //makeRuleKey(rrhs,key);	      // make key for rule entry
	  RHS key(rrhs);	// make key for rule entry
	  nonUnaryRulesP[key].push_back(rule);
	  // ===This section will be obsolete if RuleP2 is in full effect
#endif
	  addNonUnaryRuleP2(rule);
	}	
      }
    }
  }
}

void
RuleBox::setFromProbN(TM& tm)
{
  // create rules from ProbN table (ex. V-N -> N X)
  Symbol label, nodeX=sym("X");
  Symbol lhs; vector<Symbol> rhs;
  double none,right,left;	// TableN probs
  double cost;
  Rule *rule;			// new rule

  tm.RewindTableN();

  // For each ProbN entry
  while(tm.NextTableN(lhs,label,none,right,left)) {
    if (none >= Params::minI) {
      rhs.clear(); rhs.push_back(label); cost = -log(none);
      rule = newRule(lhs,rhs,cost);
      unaryRulesC[label].push_back(rule);
    }
    if (right >= Params::minI) {
      rhs.clear(); rhs.push_back(label); rhs.push_back(nodeX); cost = -log(right);
      rule = newRule(lhs,rhs,cost);
      rulesPX[label].push_back(rule);
    }
    if (left >= Params::minI) {
      rhs.clear(); rhs.push_back(nodeX); rhs.push_back(label); cost = -log(left);
      rule = newRule(lhs,rhs,cost);
      rulesXP[label].push_back(rule);
    }
  }
}

void
RuleBox::addNonUnaryRuleP2(Rule *r)
{
  Symbol s1 = r->rhs[0];
  Symbol s2 = r->rhs[1];
  if (nonUnaryRulesP2[s1][s2]==0)
    nonUnaryRulesP2[s1][s2] = new vector<Rule *>;
  nonUnaryRulesP2[s1][s2]->push_back(r);
}



//
// misc function
//

#if 0
void
RuleBox::makeRuleKey(const vector<Symbol>& rhs, string& key)
{
  key = sym(rhs[0]);
  for (int i=1; i<rhs.size(); i++) key += (" " + sym(rhs[i]));
}
#endif
