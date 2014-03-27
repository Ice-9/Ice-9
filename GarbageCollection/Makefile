
BITS = 64
BITS = 32

TARGET = lang

.SUFFIXES:
.SUFFIXES: .o .hpp .cpp

INCLAND=-I../include

GCFLAGS=-std=gnu++0x -fno-rtti
CFLAGS=-m$(BITS) -O3 $(INCLAND)


.cpp.o:
	@echo
	g++ $(CFLAGS) $(GCFLAGS) -o $*.o -c $*.cpp 2>&1 | head -20

# all: clean
all: $(TARGET) tests

clean:
	rm -f *.o *.so $(TARGET)

tests: $(TARGET)
	-ls -lF *.o *.so $(TARGET) 2>&-


qualifier.o: inlines.hpp qualifier.hpp allocator.hpp coredefs.hpp extent.hpp node.hpp standard.hpp memory9.hpp op_ptr.hpp qualifier.cpp

allocator.o: inlines.hpp qualifier.hpp allocator.hpp coredefs.hpp extent.hpp node.hpp standard.hpp memory9.hpp op_ptr.hpp allocator.cpp

extent.o:    inlines.hpp qualifier.hpp allocator.hpp coredefs.hpp extent.hpp node.hpp standard.hpp memory9.hpp op_ptr.hpp extent.cpp

node.o:      inlines.hpp qualifier.hpp allocator.hpp coredefs.hpp extent.hpp node.hpp standard.hpp memory9.hpp op_ptr.hpp node.cpp

op_ptr.o:    inlines.hpp qualifier.hpp allocator.hpp coredefs.hpp extent.hpp node.hpp standard.hpp memory9.hpp op_ptr.hpp op_ptr.cpp

libgarbo.so: extent.o node.o qualifier.o allocator.o op_ptr.o
	g++ -m$(BITS) -shared -fPIC -s -o libgarbo.so extent.o node.o qualifier.o allocator.o op_ptr.o

$(TARGET).o: $(TARGET).cpp inlines.hpp qualifier.hpp standard.hpp memory9.hpp coredefs.hpp extent.hpp node.hpp allocator.hpp op_ptr.hpp

$(TARGET): libgarbo.so $(TARGET).o
	g++ $(CFLAGS) $(GCFLAGS) -s -o $(TARGET) $(TARGET).o libgarbo.so -pthread

