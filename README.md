# MPreprocessor
Preprocessor adding protocol oriented programming, ARC, and objects to C.

This program will function as a compiler for a language I've developed which is a superset of C (I haven't named it yet).

The language supports protocol oriented programming, classes, access specifiers, method overloading and overriding, single inheritance with a universal base class similar to NSObject in Objective-C, and requires method definitions inside of class definitions similar to Swift.

The language achieves compile time polymorphism through a series of tables comparable to a vtable in C++.

The language was influenced by Python's use of PyObject for heterogenous containers, Python's powerful native types, Swift's classes and protocols, and C++'s syntax.

The implementation is essentially a pre processor which converts the supplied files into valid C code and pipes the output to the gcc c compiler.
It does this without the use of a syntax tree.

After reaching a usable state with this preprocessor I plan to add native support for several ADTs (string, map/dict, set, stack, list, queue, etc.). I will also implement a basic file i/o library and then rewrite the preprocessor in the new language with added features and bootstrap that for a working command line compiler.
