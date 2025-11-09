#
#
#

CC = cc
#for NeXTSTEP
CFLAGS = -g -I. -O0 -posix 
#For most else
CFLAGS = -I. -g

OBJECTS = acct.o      dsort.o     main.o      partab.o    sys.o       trap.o\
alloc.o     dz.o        malloc.o    pipe.o      sys1.o      tty.o\
ansidisk.o  fio.o       mba.o       prf.o       sys2.o      uba.o\
bio.o       hp.o        mem.o       prim.o      sys3.o      univec.o\
clock.o     ht.o        missing.o   rdwri.o     sys4.o      v45lnk.o\
conf.o      iget.o      mx1.o       sig.o       sysent.o\
cons.o      jcons.o     mx2.o       slp.o       tdump.o\
dkleave.o   machdep.o   nami.o      subr.o      text.o

INCLUDES = -I.

32v: $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o 32v $(OBJECTS) $(LIBS)


clean::
	rm *.o 32v
