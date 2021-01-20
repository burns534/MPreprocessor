#include "lexer.h"
#include <map>
#include <string>
#include <vector>
#include <assert.h>
#include <set>
#include <sstream>
#include <stack>

struct Parameter {
    std::vector<MToken *> specifiers;
    std::string name;
    Parameter(std::string, std::vector<MToken *>);
};

enum ItemType {
    VariableDec, VariableDef, FunctionDef
};

struct Function {
    std::vector<MToken *> specifiers;
    std::string identifier;
    std::string body;
    std::vector<Parameter> params;
    Function(std::vector<MToken *> specifiers, std::string identifier, std::vector<Parameter> params, std::string body);
    std::string generate_call(std::vector<std::string> arguments);
    void print();
};

struct Preprocessor {
    Preprocessor(std::string);
    void process(std::string);
    std::string class_declaration();
    std::string class_item(std::string super_class);
    std::string initializer();
    std::string deinitializer();
    std::vector<MToken *> declaration_specifiers();
    std::vector<Parameter> parameter_list();
    MToken * type_specifier();
    MToken * type_qualifier();
    MToken * storage_class_specifier();
private:
    int accept(int token);
    void unaccept();
    void first_pass(); // gathers all user defined types
    void second_pass(); // gathers type info for all variables
    std::string replaced_function_call(std::string scope);
    std::string is_function_definition();
    MToken **tokens;
    size_t tokenCount, cursor;
    // map from class name to hash
    std::map<std::string, std::string> class_name_table;
    // map from class name to map from function name to function struct
    std::map<std::string, std::map<std::string, Function *> > function_lookup_table;
    // map from variable name to variable type
    std::map<std::string, std::string> variable_types;
    // set of user defined type names used for determining if type is udt
    std::set<std::string> user_defined_types;
    // mapping from identifier to class owner name
    std::map<std::string, std::string> private_identifiers; 
    // mapping from class identifier to super class identifier
    std::map<std::string, std::string> super_class;
    // mapping from function identifier to class scope
    std::map<std::string, std::string> function_call_class_scope;
    // may need to change this later
    // mapping from class identifier to list of protocol identifiers
    std::map<std::string, std::vector<std::string> > protocol;
};