#include "lexer.h"
#include <map>
#include <string>
#include <vector>
#include <assert.h>
#include <set>
#include <sstream>
#include <stack>
#include "mlib.h"

struct Parameter {
    std::vector<MToken *> specifiers;
    std::string name;
    Parameter(std::string, std::vector<MToken *>);
};

enum ItemType {
    VariableDec, VariableDef, FunctionDef
};

struct Function {
    std::string scope;
    std::vector<MToken *> specifiers;
    std::string identifier;
    std::vector<MToken *> body_tokens;
    std::vector<Parameter> params;
    Function(std::string scope, std::vector<MToken *> specifiers, std::string identifier, std::vector<Parameter> params, std::vector<MToken *> body_tokens);
    // std::string generate_call(std::vector<std::string> arguments);
    void print();
};

struct Preprocessor {
    Preprocessor(std::string);
    void process(std::string);
    std::string class_declaration();
    std::string class_item(std::string super_class, bool *outside_class_body);
    std::string initializer();
    std::string deinitializer();
    std::vector<MToken *> declaration_specifiers();
    std::vector<Parameter> parameter_list();
    MToken * type_specifier();
    MToken * type_qualifier();
    MToken * storage_class_specifier();
private:
    std::string macro_text;
    int accept(int token);
    void unaccept();
    void first_pass(); // gathers all user defined types
    void second_pass(); // gathers type info for all variables. As string?
    void collect_function_arguments(size_t &i, std::vector<MToken *> tokens, std::vector<std::string> *arguments, std::vector<std::string> *argument_types);
    std::string generate_function_body(Function *function);
    std::vector<MToken *> tokens;
    size_t tokenCount, cursor;
    // map from class method hashed identifier to function info
    std::map<std::string, Function *> function_info;
    // the following two could be cleaned up with variable struct
    // map from variable name to variable class scope (original)
    std::map<std::string, std::string> variable_scope;
    // map from variable name to variable type
    std::map<std::string, std::string> variable_types;
    // mapping from identifier to original class owner name
    std::map<std::string, std::string> private_identifiers;
    // mapping from class identifier to super class identifier (original)
    std::map<std::string, std::string> super_class;
    // may need to change this later
    // mapping from class identifier to list of protocol identifiers
    std::map<std::string, std::vector<std::string> > protocol;
};