
BITS = 64
BITS = 32

TARGET = indirect

.SUFFIXES:
.SUFFIXES: .o .hpp .cpp .b4cpp

LIBLAND=../GarbageCollection
INCLAND=-I../include -I../GarbageCollection


# BEWARE: higher levels of optimisation beyond 1 move code around in big lumps
# which spoils the "region registration" code (tries to ID a memory address
# from the region it is in for debugging purposes)
# There's probably a specific few options that can be individually disabled
# ... aaannnddd.... I might have found them!?!?!?
#
GCFLAGS=-std=gnu++0x -fno-rtti $(INCLAND)

CFLAGS=-m$(BITS) -O2 -fno-cse-follow-jumps -fno-gcse -fno-strict-aliasing -fno-crossjumping -fno-reorder-blocks -falign-labels=1 -falign-loops=1 -falign-jumps=1 -fno-defer-pop -fcaller-saves -fno-inline

# if you put -s (see inside fixlabels) drop optimiser level to -O2 above
.b4cpp.cpp:
	perl ./fixlabels.pl -s -b $(BITS) < $*.b4cpp > $*.cpp

.cpp.o:
	@echo
	g++ $(CFLAGS) $(GCFLAGS) -o $*.o -c $*.cpp

# all: clean
all: tests

run: $(TARGET)
	./indirect

clean:
	rm -f *.o *.so $(TARGET) tilengine.cpp

tests: $(TARGET)
	ls -lF *.o *.so $(TARGET) 2>&-

test: $(TARGET)
	./indirect -no-ok < t/smoketest.ice9
	./indirect -no-ok < t/testcase.ice9
	./indirect -no-ok < t/test_rc4.ice9
	./indirect -no-ok < t/ansitest.ice9

smoke: $(TARGET)
	./indirect -no-ok < t/smoketest.ice9

list-words:
	egrep '^\s+(ANSI\s+)?(PRIMARY|SECONDARY)\b' *.b4cpp

# following is a little junker for putting random working tests
testy: $(TARGET)
	./indirect -no-ok < t/testy.ice9

tilengine.cpp: tilengine.b4cpp fixlabels.pl


tilengine.o: tilengine.cpp blkbuffer.hpp tilengine.hpp stacks.hpp

blkbuffer.o: blkbuffer.cpp blkbuffer.hpp tilengine.hpp stacks.hpp

rwlock.o: rwlock.cpp

$(TARGET).o: $(TARGET).cpp


libtil.so: tilengine.o blkbuffer.o rwlock.o
	g++ -m$(BITS) -shared -fPIC -o libtil.so tilengine.o blkbuffer.o rwlock.o

$(TARGET): libtil.so $(LIBLAND)/libgarbo.so $(TARGET).o
	g++ $(CFLAGS) $(GCFLAGS) -s -o $(TARGET) $(TARGET).o libtil.so -L/usr/local/lib $(LIBLAND)/libgarbo.so -pthread -lm -lreadline

