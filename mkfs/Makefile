#
#
#

CC = cc
#for NeXTSTEP
CFLAGS = -g -I. -O0 -posix 
#For most else
CFLAGS = -I. -g \
	-Wno-implicit-int \
	-Wno-implicit-function-declaration \
	-Wno-return-mismatch \
	-Wno-int-conversion

OBJECTS = mkfs.o

INCLUDES = -I.

mkfs: $(OBJECTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o mkfs $(OBJECTS) $(LIBS)


clean:
	rm *.o mkfs 
