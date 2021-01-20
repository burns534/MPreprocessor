//
//  main.cpp
//  CompilerPractice
//
//  Created by Kyle Burns on 1/8/21.
//
#include "preprocessor.h"
#include <iostream>

int main(int argc, char **argv) {
    std::string filename;
    if (argc == 2) filename = std::string(argv[1]);
    Preprocessor test = Preprocessor(filename);
    test.process("output.txt");
    return 0;
}
