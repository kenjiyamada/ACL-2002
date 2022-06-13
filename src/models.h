#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <strstream>
#include <fstream>
#include <cmath>
#include <algorithm>

#include "misc.h"

extern "C" {
  #include <math.h>
  #include "SLM2.h"
}

//
// Symbol Table
//

typedef short Symbol;

namespace SymbolTable {
  void SplitSymbol(Symbol, char, vector<Symbol>&);
  void Dump();
  int Size();
};

Symbol sym(const string& str);
string& sym(const Symbol s);


//
// Reorder
//

namespace Reorder {
  int MaxReord(int);
  int Reord(int,int,int);
  void ShowReorder(int,int);
};

int reord(int numc, int idx, int nth);
void reorder(const vector<Symbol>& in, vector<Symbol>& out, int ridx);

//
// LM
//

typedef id__t WordId;

class LM {
  ng_t ngram_model;
  inline ng_t *ng() { return &ngram_model; }
  bool inUse;

public:
  LM(const string &fname);
  WordId GetId(const string& str);
  double GetLogBigram(WordId *id);
  double GetLogBigram(WordId id1, WordId id2);
  double GetLogTrigram(WordId *id);
  double GetLogTrigram(WordId id1, WordId id2, WordId id3);
  bool InUse() { return inUse; }
};


//
// Word
//

class Word {
  // class common table
  static int maxval;
  static map<string,WordId> sidTable;
  static vector<string> strTable;
  static Word wordNULL;

  // real data
  WordId sid;			// id for strTable[]
  WordId vid;			// LM vocab id

public:
  Word(const string&, LM& lm);
  Word() { sid=0; vid=0; }
  static void Clear();
  const string& str() { return strTable[sid]; }
  int id() { return sid; }
  Word& operator=(const Word& w) { sid=w.sid; vid=w.vid; return *this; }
  //  Word& operator=(const int n) { sid=0; vid=0;  return *this; }
  bool empty() { return (sid==0); }
  bool isNULL() { return (sid==wordNULL.sid); }
  WordId LMid() { return vid; }
  void Show();
};

//
// TM
//

class TM;			// forward declaration

class EntryR {
  friend class TM;
  vector<double> ord;
};

class EntryN {
  friend class TM;
  double ins[3];
  Symbol keysym;		// == sym(key)
  Symbol label;			// this node's label
};

class EntryT {
  friend class TM;
  map<string,double> rtrans;
};


class TM {
  static const int maxMode = 4;
  int lineCnt[maxMode];

  // model tables
  map<string,EntryR> tableR;	// tableR["V_N_N"].ord[3]=prob
  map<string,EntryN> tableN;	// tableN["V-N"].ins[1]=prob
  map<string,EntryT> tableT;	// tableT["jwd"].rtrans["ewd"]=prob

  map<string,EntryN>::const_iterator iterN;

public:
  TM(const string& fname);
  void GetProbR(const vector<Symbol>&, vector<double>&);
  void GetEWords(const string, vector<string>&, vector<double>&);
  void RewindTableN() { iterN = tableN.begin(); }
  bool NextTableN(Symbol&,Symbol&,double&,double&,double&);
  void Show();
};

//
// CFG
//

class CFG;			// forward declaration

class EntryC {
  friend class CFG;
  static const int maxc = Params::MaxChildren;
  Symbol lhs;
  Symbol rhs[maxc];
  int numc;
  double prob;
public:
  EntryC() { numc=0; prob=0.0; }
};

class CFG {
  vector<EntryC> rules;
  map<string,map<Symbol,double> > lexicon; // lexicon["apple"][N]=prob

  vector<EntryC>::const_iterator iter;

public:
  CFG(const string& fname);
  void Rewind() { iter = rules.begin(); }
  bool Next(Symbol& lhs, vector<Symbol>& rhs, double& prob);
  void GetLexProbs(const string, vector<Symbol>&, vector<double>&);
  void Show();
};

#endif // MODELS_H
