//
//  main.cpp
//  CompilerPractice
//
//  Created by Kyle Burns on 1/8/21.
//
#include "preprocessor.h"
#include <iostream>

int main(int argc, char **argv) {
    std::string filename, outfile;
    if (argc > 1) {
        filename = argv[1];
        outfile = argv[2];
    } else {
        std::cerr << "Error: insufficient arguments\n";
        exit(EXIT_FAILURE);
    }
    Preprocessor test = Preprocessor(filename + ".txt");
    test.process(outfile);
    return 0;
}
