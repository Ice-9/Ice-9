Ice-9
=====

## Ice-9 core - GC object store and Indirectly threaded TIL core

The GC object store is simple for the purposes of being functional early. Loosely based on Smalltalk 80 store but does not use an Object Table. Influenced by the book "Garbage Collection: Algorithms for Automatic Dynamic Memory Management" by Richard Jones and Rafael Lins.

Minimally 32 bit due to the use of 2 attribute bits:

* 00 32 bit pointer (4 byte aligned)
* 01 30 bit integer (signed)
* 10 32 bit pointer to "String Qualifier"
* 11 32 bit pointer to "String Qualifier" used as an immutable Symbol

Capable of 64 bit builds - I've been swapping between the two on a regular basis to ensure that both word sizes are correctly catered for. 64 bit needs fatter or more regular padding, and causes some wierdness in the TIL core regarding double-cells or the low-level sized peek/poke type words - I haven't really read much about the issues that have been encountered by 64 bit implementations of FORTH. Also, I haven't yet found a 128 bit library which would cover the idea of having true 'double words' that are obligatory in FORTH.

### Main features/weaknesses of the object store

* Code is thread-safe. Each store is not intended to be shared by multiple threads - OR ARE THEY...
* Uses reference counting but also capable of a full sweep when needed.
* As suggested by the GC book, deferred deallocation is used to avoid recursive freeing. NOTE: not proven to be correct yet! NOTE: not proven to improve performance! NOTE: not proven to have low impact on memory fragmenting or have low impact on coalescence. ETC ETC ETC. NOTE: pulling the deferred deallocation would require implementing a recursive free.
* Implements the "String Qualifier" and "String Space" concept from the Icon and Unicon languages. Strings are immutable as such. String space is separately collected as needed. All the key features of the Icon string type and string space are implemented. NOTE: as per Icon, the String datatype will behave as mutable using the optimisations offered by the String Qualifier concept.
* The distinct bit-tags for an immutable Symbol simply allows special treatment of Symbols as expected by anyone familiar with Smalltalk, Ruby etc.

### Main features of the TIL core:

Started life based on the book Threaded Interpretive Languages by RG Loeliger. (I have an original copy - nyah-nyah-nyah nyah nyahhh!) Modified to try to follow ANS Forth 1994 and then lately 2012. Still working on the modern vocabulary system. Planning to add a special namespaced token search feature eg vocab1::thingy will find thingy in vocab1 regardless of the current search list. No big deal, but it would be nice.

Contains words to directly call POSIX threading functions and associated thread control objects eg mutexes etc. Each thread gets its own stack and object store. Not sure how to deal with the dictionary which currently handles low-level data storage in the fine tradition of TILs... Sharing is probably ugly. There is no code currently in existence which attempts to lock the dictionary whenever it is extended or even written. Intentions are still completely open.

NOTE: SP (and the other stack pointers) currently point at the next free slot (as per Loeliger), not at the top slot of the stack as seems to be expected by the ANS standards. Bummer. Working on that right now...

Making progress towards passing all the tests in t/ansitest.ice9 - which is not necessarily the ultimate test suite since it's kinda getting old. However, it's a damn fine start.

The TIL core is most likely not free of STATE dependencies. See http://www.complang.tuwien.ac.at/projects/forth.html for links dealing with STATE-smartness.

## To Be Continued...
