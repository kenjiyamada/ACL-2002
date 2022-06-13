#ifndef MISC_H
#define MISC_H

#include <string>

namespace Params {
  // command line options
  extern string cfgFile;
  extern string tmFile;
  extern string lmFile;
  extern double minT;
  extern double minN;
  extern double minR;
  extern double minI;
  extern double pruneBeam;
  extern int numResults;

  extern bool showTree;		// show tree for results
  extern bool dupConstit;	// no constit merging
  extern bool noCfgProb;	// ignore CFG probs
  extern bool noCfgLexProb;	// ignore CFG lex probs (use pure CFG prob)
  extern bool noNullJWords;	// not use NULL Jwords
  extern bool noPrune;	        // no pruning

  extern string cheatFile;	// cheating file name
  extern bool cheatShow;	// show cheat output (for later use)
  extern bool cheatVecAll;	// show all vector elements specified by cheat info
  extern bool cheatPrune;	// prune using cheat info
				    

  extern int verbose;
  extern int test;
  extern int debug;
};

namespace Params {
  // hard-coded params
  static const int MaxSentLen = 20;
  static const int MaxChildren = 5;
  static const int MaxReorder = 120;		// factrial of MaxChildren
  static const double MinDouble = 1e-100;

  static const int MaxUnary = 3;
  static const int MaxXConstitLen = 1;
  static const int MaxSymbols = 200;            // used by RuleBox

  void Set(int argc, char* argv[]);
  void SetThreshold(const string&,const string&,const string&,const string&);
  void Print();
};
  

//
// Error handler, etc
//

void error_exit();
void error_exit(const string& msg);
int MemSize();
double UTime();

//
// Misc Printing functions
//

inline void
ShowCost(double tmcost, double lmcost)
{
  cout << " (" << tmcost+lmcost << ") [TM=" << tmcost << " LM=" << lmcost << "]\n";
}




#endif // MISC_H
