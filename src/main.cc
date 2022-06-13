#include <string>

#include "misc.h"
#include "models.h"
#include "decode3.h"

//
// Params
//

namespace Params {
  // command line options
  string cfgFile = "default.cfg";
  string tmFile = "default.tm";
  string lmFile = "";
  double minT = 0.03;
  double minN = 0.05;
  double minR = 0.3;
  double minI = 0.2;
  double pruneBeam = 5.0;
  int numResults = 10;
  bool showTree = false;	// show tree for results
  bool dupConstit = 0;		// no constit merging
  bool noCfgProb = false;	// ignore CFG probs
  bool noCfgLexProb = false;	// ignore CFG lex probs (use pure CFG prob)
  bool noNullJWords = false;	// not use NULL Jwords
  bool noPrune = false;		// no pruning

  string cheatFile = "";	// cheating file name
  bool cheatShow = false;	// not show cheat output (for later use)
  bool cheatVecAll = false;	// not show all vector elements specified by cheat info
  bool cheatPrune = false;	// not prune using cheat info

  int verbose = 0;
  int test = 0;
  int debug = 0;

  // hard coded params are in params.h
  // function declarations are in params.h
};

void
Params::Set(int argc, char* argv[])
{
  string arg;
  int i=0;
  while(++i<argc) {
    arg=argv[i];
    if (arg=="-v") verbose=1;
    else if (arg=="-V") verbose=2;
    else if (arg=="-V3") verbose=3;
    else if (arg=="-cfg") cfgFile=argv[++i];
    else if (arg=="-tm") tmFile=argv[++i];
    else if (arg=="-lm") lmFile=argv[++i];
    else if (arg=="-xt") minT=atof(argv[++i]);
    else if (arg=="-xn") minN=atof(argv[++i]);
    else if (arg=="-xr") minR=atof(argv[++i]);
    else if (arg=="-xi") minI=atof(argv[++i]);
    else if (arg=="-beam") pruneBeam=atof(argv[++i]);
    else if (arg=="-n") numResults=atoi(argv[++i]);
    else if (arg=="-tree") showTree=1;
    else if (arg=="-dup") dupConstit=1;
    else if (arg=="-nocfgprob") noCfgProb=true;
    else if (arg=="-usecfgprob") noCfgProb=false;
    else if (arg=="-nocfglexprob") noCfgLexProb=1;
    else if (arg=="-nonulljwords") noNullJWords=1;
    else if (arg=="-noprune") noPrune=1;
    else if (arg=="-cheat") cheatFile=argv[++i];
    else if (arg=="-cheatvec") cheatVecAll=1;
    else if (arg=="-cheatshow") cheatShow=1;
    else if (arg=="-cheatprune") cheatPrune=1;
    else if (arg=="-test") test=1;
    else if (arg=="-debug") debug=1;
    else {
      cerr << "Unrecognized option: " << arg << "\n";
	exit(1);
    }
  }
}

void
Params::SetThreshold(const string& xt, const string& xn,
		     const string& xr, const string& xi)
{
  minT=atof(xt.c_str()); minN=atof(xn.c_str());
  minR=atof(xr.c_str()); minI=atof(xi.c_str());
}

void
Params::Print()
{
  cout << "Verbose: " << verbose << "\n";
  cout << "cfgFile: " << cfgFile;
  cout << ", tmFile: " << tmFile;
  cout << ", lmFile: " << lmFile << "\n";
  cout << "Beam: " << pruneBeam << "\n";
  cout << "minT: " << minT;
  cout << ", minN: " << minN;
  cout << ", minR: " << minR;
  cout << ", minI: " << minI << "\n";
  cout.flush();
}


//
// Error handler
//

void
error_exit()
{
  exit(1);
}

void
error_exit(const string& msg)
{
  cerr << msg << "\n";
  exit(1);
}

//
// Process Info (CPU & Memory Usage)
//

extern "C" {
#include <sys/times.h>
}

double
UTime() {
  struct tms tbuf;
  times(&tbuf);
  return ((double)tbuf.tms_utime/CLK_TCK);
}

#ifdef sun

extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
}

int 
MemSize()
{
  struct stat buf;
  int pid = getpid();
  char fname[32];
  sprintf(fname,"/proc/%d",pid);
  stat(fname,&buf);
  return buf.st_size;
}

#else // not sun (Linux)

extern "C" {
#include <stdio.h>
#include <unistd.h>
}

int
MemSize()
{
  int pid = getpid();
  char fname[32];
  sprintf(fname,"/proc/%d/status",pid);
  ifstream in(fname);
  if (!in) error_exit("cannot open [" + string(fname) + "]");

  string VmSize;

  string line,w; vector<string> item;
  while(getline(in,line)) {
    istrstream ist(line.c_str());
    item.clear();
    while(ist>>w) item.push_back(w);
    if (item.size()>0) {
      if (item[0]=="VmSize:") {
	VmSize = item[1]; break;
      }
    }
  }
  int ret = atoi(VmSize.c_str());
  return ret;
}
#endif // sun

void
ShowMemory()
{
  cout << "MemSize=" << MemSize();
  cout << ",CPUTime=" << UTime() << "\n";
  cout.flush();
}

//
// main
//
int
main(int argc, char* argv[])
{
  Params::Set(argc,argv);
  if (Params::verbose) Params::Print();

  TM tm(Params::tmFile);
  tm.Show();
  CFG cfg(Params::cfgFile);
  cfg.Show();

  LM lm(Params::lmFile);
  if (Params::verbose) ShowMemory();

  Decoder dc(lm,tm,cfg);
  if (Params::verbose) cout << "#Symbol=" << SymbolTable::Size() << "\n";
  dc.DecodeInputs(cin);
  if (Params::verbose) ShowMemory();
}
  
