CHARMC=/home/ralfgunter/uiuc/curr/cs598lvk/charm/bin/charmc $(OPTS)
OPTS = -I/home/ralfgunter/uiuc/curr/cs598lvk/LSHBOX/include -std=c++11
TESTOPTS = ++local
 
OBJS = superRes.o
 
 
all: superRes superRes_prj
 
superRes: $(OBJS)
	$(CHARMC) -g -language charm++ -o superRes $(OBJS)
 
superRes_prj: $(OBJS)
	$(CHARMC) -O3 -language charm++ -tracemode projections -lz -o superRes.prj $(OBJS)
   
SuperResolution.decl.h:  superRes.ci
	$(CHARMC)  superRes.ci
 
clean:
	rm -f *.decl.h *.def.h conv-host *.o superRes superRes.prj charmrun 
 
cleanp:
	rm -f *.sts *.gz *.projrc *.topo *.out
 
superRes.o: superRes.C SuperResolution.decl.h
	$(CHARMC) -c -g superRes.C
 
test: all
	./charmrun superRes +p4
