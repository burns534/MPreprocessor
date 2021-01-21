# MPreprocessor
Preprocessor adding protocol oriented programming and objects to C

This program will function as a compiler for a language I've developed which is a superset of C (I haven't named it yet).

The language supports protocol oriented programming, access specifiers, classes, single inheritance with a universal base class similar to 
NSObject in Objective-C, and requires method definitions inside of class definitions similar to Swift or Python.

The language was influenced by Python's use of PyObject for heterogenous containers, Python's powerful native types, Swift's classes and protocols, and C++'s syntax.

The implementation is essentially a pre processor which converts the supplied files into valid C code and pipes the output to the gcc c compiler.
It does this without the use of a syntax tree.

After completing this preprocessor I plan to add native types like string, map/dict, stack, list, queue, etc. I will also implement a basic file i/o library and
then rewrite the program in the new language and compile for a usable result.
