
SRCS = main.cc decode3.cc models.cc rules.cc constit.cc cheat.cc
HEADERS = misc.h decode3.h models.h rules.h constit.h
OBJS = main.o decode3.o models.o rules.o constit.o cheat.o
TESTS = tst.egrm tst.mdl tst.jsnt tst.out mus4.min.out mus4.minl.out test3-np.out

REALOPTS = -nocfgprob -cfg D2/je2k.peg -tm D2/je2k.20.out -lm D2/news.binlm
REALIN = D2/mus4.jsnt
USECHEAT = -cheat mus4.cheat
USECHEAT_CFG = -cheat mus4.cheat.cfg

TESTOUTS = mus4.cheat mus4.out-c mus4.out-c.v mus4.out.ref mus4.tree.ref mus4.tree mus4.cheat.cfg mus4.out-c.cfg mus4.out-c.v+ mus4.tree+


decode3 : ${OBJS} #Makefile
	g++ ${CFLAGS} -o decode3 ${OBJS} -L. -lSLM2


%.o : %.cc
	g++ ${CFLAGS} -c -o $@ $<

main.o : main.cc misc.h models.h decode3.h rules.h constit.h

decode3.o : decode3.cc decode3.h models.h rules.h constit.h misc.h

models.o : models.cc models.h misc.h

rules.o : rules.cc rules.h models.h misc.h

constit.o : constit.cc constit.h models.h misc.h

cheat.o : cheat.cc decode3.h models.h rules.h constit.h misc.h

debug :
	make CFLAGS=-g

opt :
	make CFLAGS=-O3

clean :
	rm -f decode3 *.o *~ core

tar :
	tar cvfz all.tgz ${SRCS} ${HEADERS} ${TESTS} Makefile Memo

print :
	enscript -h -2r misc.h main.cc rules.h rules.cc models.h models.cc constit.h constit.cc decode3.h decode3.cc cheat.cc

##############################################

test1 : decode3
	# check Params
	decode3 -v -test -xt 1.2e-3 -cfg tst.egrm -tm tst.mdl < /dev/null

test2 : decode3
	# check minimum CFG/TM
	decode3 -cfg tst.egrm -tm tst.mdl < tst.jsnt | diff tst.out - && echo Test2 OK

test3 : decode3
	# check minimum CFG but large TM
	decode3 -nocfgprob -cfg D2/mus4.peg -tm D2/je2k.20.out < D2/mus4.jsnt | diff mus4.min.out - && echo Test3 OK

test3-np : decode3
	# check minimum CFG but large TM without pruning
	decode3 -noprune -nocfgprob -cfg D2/mus4.peg -tm D2/je2k.20.out < D2/mus4.jsnt | diff test3-np.out - && echo Test3-np OK

test4 : decode3
	# check LM
	decode3 -nocfgprob -cfg D2/mus4.peg -tm D2/je2k.20.out -lm D2/musd.binlm < D2/mus4.jsnt | diff mus4.minl.out - && echo Test4 OK

testall : test1 test2 test3 test3-np test4
	@ echo === Test ALL OK ===


##############################################

real : decode3
	decode3 -v ${REALOPTS} < ${REALIN} > mus4.log

minref : decode3
	decode3 ${REALOPTS} -cfg D2/mus4.peg < ${REALIN} 



cheat4 : mus4.cheat

usecheat : mus4.out-c mus4.cheat

usecheat-v : mus4.out-c.v mus4.cheat

cheatref : mus4.out.ref mus4.cheat

treeref : mus4.tree.ref

realtree : mus4.tree

cheat4cfg : mus4.cheat.cfg

usecfg : mus4.out-c.cfg mus4.cheat.cfg

usecfgvec : mus4.out-c.v+ mus4.cheat.cfg

realtreecfg : mus4.tree+



mus4.cheat : decode3
	decode3 -cheatshow ${REALOPTS} -cfg D2/mus4.peg < ${REALIN} > mus4.cheat

mus4.out-c : decode3
	decode3 ${USECHEAT} ${REALOPTS} < ${REALIN} > mus4.out-c

mus4.out-c.v : decode3
	decode3 -v ${USECHEAT} ${REALOPTS} < ${REALIN} > mus4.out-c.v

mus4.out.ref : decode3
	decode3 -v ${USECHEAT} ${REALOPTS} -cfg D2/mus4.peg < ${REALIN} > mus4.out.ref

mus4.tree.ref : decode3
	decode3 -tree ${REALOPTS} -cfg D2/mus4.peg < ${REALIN} > mus4.tree.ref

mus4.tree : decode3
	decode3 -tree ${REALOPTS} < ${REALIN} > mus4.tree

mus4.cheat.cfg : decode3
	decode3 -cheatshow ${REALOPTS} -cfg D2/mus4.peg -usecfgprob < ${REALIN} > mus4.cheat.cfg

mus4.out-c.cfg : decode3
	decode3 ${USECHEAT_CFG} ${REALOPTS} -usecfgprob < ${REALIN} > mus4.out-c.cfg

mus4.out-c.v+ : decode3
	decode3 -cheatvec ${USECHEAT_CFG} ${REALOPTS} -usecfgprob < ${REALIN} > mus4.out-c.v+

mus4.tree+ : decode3
	decode3 -tree ${REALOPTS} -usecfgprob < ${REALIN} > mus4.tree+



cleanrealtest :
	rm -f ${TESTOUTS}

saverealtest :
	mv ${TESTOUTS} save/



testrealall : decode3 ${TESTOUTS}
	@ diff mus4.cheat TestC2/mus4.cheat.lmC2 >/dev/null && echo cheat4 OK
	@ diff mus4.out-c TestC2/x.lmC2 >/dev/null && echo usecheat OK
	@ diffx mus4.out-c.v TestC2/xx.lmC2 >/dev/null && echo usecheat-v OK
	@ diffx mus4.out.ref TestC2/xx.ref.lmC2 >/dev/null && echo cheatref OK
	@ diff mus4.tree.ref TestC2/tree.ref.lmC2 >/dev/null && echo treeref OK
	@ diff mus4.tree TestC2/tree.lmC2 >/dev/null && echo realtree OK
	@ diff mus4.cheat.cfg TestC2/mus4.cheat.lmC2+cfg >/dev/null && echo cheat4cfg OK
	@ diff mus4.out-c.cfg TestC2/x.lmC2+cfg >/dev/null && echo usecfg OK
	@ diff mus4.out-c.v+ TestC2/x.all.lmC2+cfg >/dev/null && echo usecfgvec OK
	@ diff mus4.tree+ TestC2/tree.lmC2+cfg >/dev/null && echo realtreecfg
	@ rm -f diffx.tmp1 diffx.tmp2
	@ echo === ALL REAL TEST OK ===
