Ice-9
=====

# MANIFESTO

The symbol / (looks like a tick if you squint) means something that's doable.

The symbol ? means something that's going to take effort.

* Distinct syntax for distinct features
* ? OO robustness, Eiffel style
 * ? Object Roles - add rigorous dynamics to OOP. Research is sparse out there...
* Objects are Good, Values are Sacred
 * values would be like Eiffel's expanded types
* / Generators and goal-directed evaluation (GDE) in the style of Icon/Unicon
 * / Failure is an option
 * / Many successes is an option
* / Threading in the style of Erlang
* ? Container-based parallelism
 * ? True relational?
 * ? need to decide how values and inheritance can co-exist
 * ? need to decide how it interacts with GDE
* / Operators in the FP sense
* / Closures, lambdas, currying
 * ? Lazy evaluation?
* ? Dimension aware (eg physical dims such as time, length. Mostly a compiler issue)
 * ? does it make sense to declare your own dimensions?
 * ? what can they be called? 'metrics'? the word dimension has prior meaning in programming...

### other ideas

* binding Prolog style?
* guards in the functional style?

# Initial Activity

* Prepare an object manager

* Prepare a useful runtime core to build and experiment. Not sure if a TIL is really a good core, but apparently it worked for Actor. I liked it anyway... Other obvious targets are Java bytecodes, CIL or even Parrot.

# What's in the repo from birth?

## Ice-9 core - GC object store and Indirectly threaded TIL core

The GC object store is simple for the purposes of being functional early. Loosely based on Smalltalk 80 store but does not use an Object Table. Influenced by the book "Garbage Collection: Algorithms for Automatic Dynamic Memory Management" by Richard Jones and Rafael Lins.

Minimally 32 bit due to the use of 2 attribute bits:

* 2r00 - 32 bit pointer (4 byte aligned)
* 2r01 - 30 bit integer (signed)
* 2r10 - 32 bit pointer to "String Qualifier"
* 2r11 - 32 bit pointer to "String Qualifier" used as an immutable Symbol

Capable of 64 bit builds - I've been swapping between the two on a regular basis to ensure that both word sizes are correctly catered for. 64 bit needs fatter or more regular padding, and causes some wierdness in the TIL core regarding double-cells or the low-level sized peek/poke type words - I haven't really read much about the issues that have been encountered by 64 bit implementations of FORTH. Also, I haven't yet found a 128 bit library which would cover the idea of having true 'double words' that are obligatory in FORTH.

### Main features/weaknesses of the object store

* Code is thread-safe. Each store is not intended to be shared by multiple threads - OR ARE THEY...
* Uses reference counting but also capable of a full sweep when needed.
* As suggested by the GC book, deferred deallocation is used to avoid recursive freeing.
 * NOTE: not proven to be correct yet!
 * NOTE: not proven to improve performance!
 * NOTE: not proven to have low impact on memory fragmenting or have low impact on coalescence! ETC ETC ETC.
 * NOTE: pulling the deferred deallocation would require implementing a recursive free.
* Implements the "String Qualifier" and "String Space" concept from the Icon and Unicon languages. Strings are immutable as such. String space is separately collected as needed. All the key features of the Icon string type and string space are implemented. NOTE: as per Icon, the String datatype will behave as mutable using the optimisations offered by the String Qualifier concept.
* The distinct 2r11 bit-tag for an immutable Symbol simply allows special treatment of Symbols as expected by anyone familiar with Smalltalk, Ruby etc.

### Main features of the TIL core

Depends on GCC because it's using computed goto's by exploiting the 'address-of-label' operator &&label. Damn you Microsoft for poo-pooing the concept. I'd otherwise like to make the source work with both these major compilers and platforms. Can't really see a sweet way to replace it all with a giant switch() yet. Are there still any other DOS/Windows compilers out there with a heartbeat that also happen to support this?

Started life based on the book Threaded Interpretive Languages by RG Loeliger. (I have an original copy - nyah-nyah-nyah nyah nyahhh!) Modified to try to follow ANS Forth 1994 and then lately 2012.

Making progress towards passing all the tests in t/ansitest.ice9 - which is not necessarily the ultimate test suite since it's kinda getting old. However, it's a damn fine start. As the comment in that file says:

    \ (C) 1993 JOHNS HOPKINS UNIVERSITY / APPLIED PHYSICS LABORATORY

Still working on modernising the Dictionary code. This is one reason why I don't trust the executability of this initial checkin, since this code is in flux.

Planning to add a special namespaced token search feature eg vocab1::thingy will find thingy in vocab1 regardless of the current search list. No big deal, but it would be nice.

Contains words to directly call POSIX threading functions and associated thread control objects eg mutexes etc. Each thread gets its own stack and object store. Not sure how to deal with the dictionary which currently handles low-level data storage in the fine tradition of TILs... Sharing is probably ugly. There is no code currently in existence which attempts to lock the dictionary whenever it is extended or even written. Intentions are still completely open. There are some multi-threading FORTH's out there, and they seem to give each thread it's own 'USER' variable space - presumably replacing the variable components of the dictionary.

Implements a floating point stack (FP) and also an object stack (OS). The OS is integrated (?prove it?) with the Object Manager. Unfortunately I don't yet have a clear set of guidelines for handing the reference counts or appropriate handling of objects because I got distracted by the modernisation of the Dictionary code.

NOTE: SP (and the other stack pointers) currently point at the next free slot (as per Loeliger), not at the top slot of the stack as seems to be expected by the ANS standards. Bummer. Working on that right now... This is one reason why I don't trust the executability of this initial checkin, since the Stack class and dependent code is in flux.

The TIL core is most likely not free of STATE dependencies. See http://www.complang.tuwien.ac.at/projects/forth.html for links dealing with STATE-smartness.

## To Be Continued...
