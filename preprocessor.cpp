#include "preprocessor.h"
#define DEBUG 1
// includes const
static bool is_type_specifier(int token) {
    return token == VOID || token == CHAR
        || token == INT || token == LONG
        || token == FLOAT || token == DOUBLE
        || token == SIGNED || token == UNSIGNED
        || token == TYPE_NAME || token == CONST;
}

static void error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}
// will need to be changed
static std::string unique_function_identifier(std::string scope, std::string identifier, std::vector<Parameter> params) {
    std::string result = "_" + scope + "_" + identifier;
    for (size_t i = 0; i < params.size(); i++) {
        for (size_t j = 0; j < params[i].specifiers.size(); j++) {
            result.push_back('_');
            result += params[i].specifiers[j]->value;
        }
    }
    return result;
}

static std::string unique_function_identifier(std::string scope, std::string identifier, std::vector<std::string> arguments) {
    std::string result = "_" + scope + "_" + identifier;
    for (size_t i = 0; i < arguments.size(); i++) {
        result += "_" + arguments[i];
    }
    return result;
}

Parameter::Parameter(std::string identifier, std::vector<MToken *>specifiers) {
    this->name = identifier;
    this->specifiers = specifiers;
}

Function::Function(std::string scope, std::vector<MToken *> specifiers, std::string identifier, std::vector<Parameter> params, std::vector<MToken *> body_tokens) {
    this->scope = scope;
    this->body_tokens = body_tokens;
    this->identifier = identifier;
    this->params = params;
    this->specifiers = specifiers;
}

void Function::print() {
    printf("Class: %s\n", identifier.c_str());
    printf("specifiers: ");
    for(size_t i = 0; i < specifiers.size(); i++) {
        specifiers[i]->print();
    }
}

// std::string Function::generate_call(std::vector<std::string> arguments) {
//     std::string result = identifier + "(";
//     assert(arguments.size() == params.size());
//     for (size_t i = 0; i < arguments.size(); i++) {
//         result += arguments[i] + (i < arguments.size() - 1 ? ", " : "");
//     }
//     result.push_back(')');
//     return result;
// }


Preprocessor::Preprocessor(std::string filename) {
    FILE *fp = fopen(filename.c_str(), "r");
    if (fp) {
        this->tokens = lex(fp, &tokenCount);
        #if DEBUG
        printf("tokenCount: %lu\n", tokenCount);
        for (size_t i = 0; i < tokenCount; i++)
            printf("%s\n", stringRepresentation(tokens[i]));
        #endif
    } else {
        fprintf(stderr, "Error with opening file %s\n", filename.c_str());
        exit(EXIT_FAILURE);
    }
    cursor = 0;
}
// have to do a preliminary pass to gather user-defined types
// second pass to gather type info for variables
void Preprocessor::process(std::string outfile) {
    first_pass(); // gather user defined types
    second_pass(); // gather type info for all variables
    std::string class_definition_string, result, scope, temp;
    std::vector<MToken *> new_tokens;
    // third pass - replace class definitions with appropriate code
    for(cursor = 0; cursor < tokenCount; cursor++) {
        if (!(temp = class_declaration()).empty()) {
            class_definition_string += temp + "\n";
        } else {
            new_tokens.push_back(tokens[cursor]);
        }
    }

    std::vector<MToken *> specifiers;
    Function * function_ptr = nullptr;
    // replace variable and method usage with appropriate name and pointers
    for (cursor = 0; cursor < new_tokens.size(); cursor++) {
        if (new_tokens[cursor]->type == IDENTIFIER) {
            std::string scope, identifier, variable_identifier = new_tokens[cursor]->value;
            // 2 cases
            int type = new_tokens[cursor + 1]->type;
            if ((type == PTR_OP || type == '.') && new_tokens[cursor + 2]->type == IDENTIFIER) {
                identifier = new_tokens[cursor + 2]->value;
                scope = variable_types[variable_identifier];
                if (scope.empty()) {
                    result += new_tokens[cursor]->value;
                    continue;
                }
                std::vector<std::string> arguments;
                std::vector<std::string> argument_types;
                // function
                if (new_tokens[cursor + 3]->type == '(') {
                    collect_function_arguments(cursor += 3, new_tokens, &arguments, &argument_types);
                    // generate full method identifier and make sure it's valid
                    identifier = unique_function_identifier(scope, identifier, argument_types);
                    if (function_info[identifier]) {
                        result += identifier + (type == PTR_OP ? "(" : "(&") + variable_identifier;
                        for (size_t j = 0; j < arguments.size(); j++) {
                            result += ", " + arguments[j];
                        }
                        result += ")";
                    } else {
                        error("Invalid method identifier 1");
                    }
                } else { // variable
                    cursor += 2;
                    result += type == PTR_OP ? "->" : ".";
                    result += identifier;
                }
            } else {
                result += variable_identifier + " ";
            }
        } else if (new_tokens[cursor]->type == TYPE_NAME) {
            printf("it was a type name");
            // add static handling here
        } else {
            printf("token/type: %s/%d\n", new_tokens[cursor]->value, new_tokens[cursor]->type);
            result += new_tokens[cursor]->value;
            result.push_back(' ');
        }
    }
    std::string output = class_definition_string + result;
    // need to make lexer lex a string, not just a file
    FILE *fp = fopen(outfile.c_str(), "w+");
    fwrite(output.c_str(), 1, output.length(), fp);
    fclose(fp);
}
// forget this for now
static std::string hash(std::string s) {
    std::string result;
    for (size_t i = 0; i < result.length(); i++) {
        std::stringstream s;
        break;
    }
    return s;
}
// add jumbled stuff later
static std::string generate_function_name(std::string class_name, std::string func_name, std::vector<std::string> param_types) {
    std::string result = "_" + class_name + "_" + func_name;
    for (size_t i = 0; i < param_types.size(); i++) {
        result += "_" + param_types[i];
    }
    return result;
}
// add inheritance later
std::string Preprocessor::class_declaration() {
    puts("class declaration called");
    bool deinit_found_flag = false;
    if (accept(CLASS) && accept(IDENTIFIER)) {
        #if DEBUG
            puts("inside class declaration");
        #endif
        std::string super_class_identifier, identifier, item_string, function_declarations, class_body = "typedef struct {";
        if (accept('{')) {
            identifier = tokens[cursor - 2]->value;
            super_class_identifier = "MObject";
        } else if (accept(':') && accept(IDENTIFIER) && accept('{')) {
            super_class_identifier = tokens[cursor - 2]->value;
            identifier = tokens[cursor - 4]->value;
        } else {
            error("Error: Syntax error");
        }

        
        identifier = "_" + hash(identifier);
        // class_name_table[identifier]
        super_class[identifier] = super_class_identifier;

        while(1) {
            if (!(item_string = class_item(identifier)).empty()) {
                printf("item string: %s\n", item_string.c_str());
                // if back of string is semicolon, it's a variable dec
                if (item_string[item_string.length() - 1] == ';') {
                    class_body += item_string;
                } else {
                    function_declarations += item_string;
                }
            } else if (!(item_string = initializer()).empty()) {
                function_declarations += item_string;
            } else if (!deinit_found_flag && !(item_string = deinitializer()).empty()) {
                deinit_found_flag = true;
                function_declarations += item_string;
            } else if (deinit_found_flag && !(item_string = deinitializer()).empty()) {
                error("Error: cannot have multiple deinitializers");
            } else if (accept('}')) {
                return function_declarations + class_body + "} _" + hash(identifier) + ";";
            } else {
                error("Class contains invalid stuff");
            }
        }
    } else {
        return "";
    }
}
// Variable definition has to be pasted into the generated initializer later
// I will implement that at a later time
// I still have to implement part of the preprocessor myself if that is going to work
std::string Preprocessor::class_item(std::string class_name) {
    #if DEBUG
        puts("class_item");
    #endif
    std::string result;
    std::vector<MToken *> specifiers = declaration_specifiers();
    if (specifiers.size() > 0) {
        // check for return type
        bool contains_return_type = false;
        bool is_private = false;
        std::string identifier;
        for (size_t i = 0; i < specifiers.size(); i++) {
            if (!contains_return_type && is_type_specifier(specifiers[i]->type)) {
                contains_return_type = true;
                break;
            } else if (!is_private && specifiers[i]->type == PRIVATE) {
                is_private = true;
            } else if (is_private && specifiers[i]->type == PRIVATE) {
                error("Error: multiple access specifiers");
            }
        }
        assert(contains_return_type);

        // get identifier
        if (accept(IDENTIFIER)) {
            identifier = tokens[cursor - 1]->value;
        } else {
            error("Error: No identifier");
        }

        if (is_private) {
            private_identifiers[identifier] = class_name;
        }

        for(size_t i = 0; i < specifiers.size(); i++) {
            result += std::string(specifiers[i]->value) + " ";
            #if DEBUG
                specifiers[i]->print();
            #endif
        }
        // distinguish between variable dec, variable def, or function def
        if (accept(';')) { // variable declaration
            result += identifier;
            variable_types[identifier] = class_name;
            printf("variable: scope -> %s: %s\n", identifier.c_str(), class_name.c_str());
            return result + ";";
        } else if (accept(',')) {
            result += identifier;
            variable_types[identifier] = class_name;
            printf("variable: scope -> %s: %s\n", identifier.c_str(), class_name.c_str());
            for (;cursor < tokenCount; cursor++) {
                if (accept(';')) {
                    break;
                } else if (tokens[cursor]->type == IDENTIFIER) {
                    variable_types[tokens[cursor]->value] = class_name;
                    result += ", ";
                    result += tokens[cursor]->value;
                } else if (tokens[cursor]->type == ',') {
                    continue;
                } else {
                    error("Invalid token in variable declaration list");
                }
            }
            return result + ";";
        } else if (accept('(')) { // function definition
            std::vector<Parameter> params = parameter_list();
            std::string method_name, parameter_string = class_name + " *self";
            if (!params.empty()) parameter_string += ", ";
            for (size_t i = 0; i < params.size(); i++) {
                for (size_t j = 0; j < params[i].specifiers.size(); j++) {
                    parameter_string += params[i].specifiers[j]->value;
                    parameter_string.push_back(' ');
                }
                parameter_string += params[i].name;
                parameter_string += i < params.size() - 1 ? ", " : "";
            }
            if (accept(')') && accept('{')) {
                // get body tokens
                size_t open_brack_count = 1;
                std::vector<MToken *> body_tokens;
                while(1) {
                    if (accept('{')) {
                        open_brack_count++;
                    } else if (accept('}')) {
                        if (--open_brack_count == 0) {
                            break;
                        }
                    } else {
                        body_tokens.push_back(tokens[cursor++]);
                    }
                }
                method_name = unique_function_identifier(class_name, identifier, params);
                printf("method_name: %s\n", method_name.c_str());
                result += method_name;
                Function *function = new Function(class_name, specifiers, identifier, params, body_tokens);
                function_info[method_name] = function;
                result += "(" + parameter_string + ") {" + generate_function_body(function) + "}\n";
                return result;
            } else {
                error("Error: Syntax error in function definition");
            }
        } else if (accept('=')) {
            // push to a queue for initialization inside initializer then return result
//TODO: skip for now
            error("Error: Not implemented yet");
        } else {
            error("Error: Invalid token");
        }
    } else {
        return result;
    }
    return result;
}

std::string Preprocessor::initializer() {
    std::string result;
    if (accept(INIT)) {
        printf("init\n");
    }
    return result;
}

std::string Preprocessor::deinitializer() {
    std::string result;
    if (accept(DEINIT)) {
    printf("deinit\n");
    }
    return result;
}

std::vector<MToken *> Preprocessor::declaration_specifiers() {
    #if DEBUG
        puts("declaration_specifiers");
    #endif
    std::vector<MToken *> result;
    MToken *token = nullptr;
    while(1) {
        if ((token = type_qualifier())) {
            result.push_back(token);
        } else if ((token = storage_class_specifier())) {
            result.push_back(token);
        } else if ((token = type_specifier())) {
            result.push_back(token);
        } else if (accept('*')) {
            result.push_back(new MToken('*', "*"));
        } else {
            return result;
        }
    }
}

std::vector<Parameter> Preprocessor::parameter_list() {
    std::vector<Parameter> result;
    std::vector<MToken *> specifiers;
    std::string identifier;
    while(1) {
        specifiers = declaration_specifiers();
        if (specifiers.size() > 0) {
            if (accept(IDENTIFIER)) {
                identifier = tokens[cursor - 1]->value;
                result.push_back(Parameter(identifier, specifiers));
                if (accept(',')) {
                    continue;
                } else {
                    return result;
                }
            } else {
                error("Error: expected identifier");
            }
        } else {
            return result;
        }
    }
}

MToken * Preprocessor::type_specifier() {
    if (accept(VOID) || accept(CHAR)
    || accept(SHORT) || accept(INT) 
    || accept(LONG) || accept(FLOAT) 
    || accept(DOUBLE) || accept(SIGNED)
    || accept(UNSIGNED)) {
        return tokens[cursor - 1];
    } else if (accept(IDENTIFIER)) {
        std::string key = tokens[cursor - 1]->value;
        if (user_defined_types.find(key) != user_defined_types.end()) {
            printf("type specifier returning valid token: %s\n", user_defined_types.find(key)->c_str());
            return new MToken(TYPE_NAME, user_defined_types.find(key)->c_str());
        } else {
            printf("\n\n\nunaccepted identifier: %s\n", key.c_str());
            unaccept();
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

MToken * Preprocessor::type_qualifier() {
    if (accept(PRIVATE) || accept(CONST) || accept(VOLATILE)) {
        return tokens[cursor - 1];
    } else {
        return nullptr;
    }
}

MToken * Preprocessor::storage_class_specifier() {
    if (accept(TYPEDEF) || accept(EXTERN) || accept(STATIC) || accept(AUTO) || accept(REGISTER)) {
        return tokens[cursor - 1];
    } else {
        return nullptr;
    }
}

int Preprocessor::accept(int token) {
    if (tokens[cursor]->type == token) {
        #if DEBUG
            printf("Accepted token %d\n", token);
        #endif
        cursor++;
        return 1;
    } else {
        return 0;
    }
}

void Preprocessor::unaccept() {
    assert(--cursor - 1 >= 0);
    #if DEBUG
        printf("unaccepted token %d\n", tokens[cursor]->type);
    #endif
}

// currently do not allow declaration list
// anonymous classes or class typedefs are not allowed
void Preprocessor::first_pass() {
#if DEBUG
    puts("first_pass called");
#endif
    while(cursor < tokenCount) {
        int token = tokens[cursor]->type;
        printf("token: %d\n", token);
        if (token == CLASS || token == STRUCT || token == UNION || token == ENUM) {
            assert(tokens[++cursor]->type == IDENTIFIER);
            user_defined_types.insert(std::string(tokens[cursor]->value));
        } else if (tokens[cursor]->type == TYPEDEF) {
            token = tokens[++cursor]->type;
            if (token == STRUCT || token == UNION || token == ENUM) {
                assert(tokens[++cursor]->type == '{');
                int open_count = 1;
                while(open_count > 0) {
                    cursor++;
                    if (tokens[cursor]->type == '{') {
                        open_count++;
                    } else if (tokens[cursor]->type == '}') {
                        open_count--;
                    }
                }
                assert(tokens[++cursor]->type == IDENTIFIER);
                user_defined_types.insert(std::string(tokens[cursor]->value));
                while(1) {
                    if (tokens[++cursor]->type == ',') {
                        assert(tokens[++cursor]->type == IDENTIFIER);
                        user_defined_types.insert(std::string(tokens[cursor]->value));
                    } else {
                        break;
                    }
                }
            } else {
                while (cursor++ && tokens[cursor]->type != ';');
                user_defined_types.insert(std::string(tokens[cursor - 1]->value));
            }
        }
        cursor++;
    }
    puts("user defined types: ");
    for (std::set<std::string>::iterator it = user_defined_types.begin(); it != user_defined_types.end(); it++) {
        printf("%s\n", it->c_str());
    }
    cursor = 0;
}
// look for variable definitions and record type
void Preprocessor::second_pass() {
#if DEBUG
    puts("second_pass called");
#endif
    while(cursor < tokenCount) {
        std::vector<MToken *> specifiers = declaration_specifiers();
        std::string identifier;
        if (accept(IDENTIFIER)) {
            identifier = tokens[cursor - 1]->value;
            if (accept(';') || accept('=') || accept(',')) {
                if (specifiers.size() > 0) {
                    std::string type;
                    for (size_t i = 0; i < specifiers.size(); i++) {
                        if (is_type_specifier(specifiers[i]->type) || specifiers[i]->type == '*') {
                            if (!type.empty()) type += " ";
                            type += specifiers[i]->value;
                        }
                    }
                    variable_types[identifier] = type;

                    if (tokens[cursor - 1]->type == ',') {
                        for (;cursor < tokenCount; cursor++) {
                            if (tokens[cursor]->type == IDENTIFIER) {
                                variable_types[tokens[cursor]->value] = type;
                            } else if (tokens[cursor]->type == ',') {
                                continue;
                            } else {
                                break;
                            }
                        }
                    }
                    continue;
                }
            }
        } 
        cursor++;
    }
    for (std::map<std::string, std::string>::iterator it = variable_types.begin(); it != variable_types.end(); it++) {
        printf("%s : %s\n\n", it->first.c_str(), it->second.c_str());
    }
    cursor = 0;
}
// have to be able to pass scope
// don't forget to check super class vtable if not present
std::string Preprocessor::replaced_function_call(std::string scope) {
    #if DEBUG
        puts("replaced_function_call");
    #endif
    std::string result;
    std::vector<MToken *> specifiers;
    Function * func;
    if (accept(IDENTIFIER)) {
        std::string func_name = tokens[cursor - 1]->value;
    }
    return result;
}

void Preprocessor::collect_function_arguments(size_t &i, std::vector<MToken *> tokens, std::vector<std::string> *arguments, std::vector<std::string> *argument_types) {
    for(; tokens[i]->type != ')' && i < tokens.size(); i++) {
        if (tokens[i]->type == IDENTIFIER) {
            arguments->push_back(tokens[i]->value);
            argument_types->push_back(variable_types[tokens[i]->value]);
        } else if (tokens[i]->type == STRING_LITERAL) {
            arguments->push_back(tokens[i]->value);
            argument_types->push_back("const_char_*");
        } else if (tokens[i]->type == CONSTANT) {
            arguments->push_back(tokens[i]->value);
            argument_types->push_back("int");
        }
    }
}
// this needs a lot of work....
// first need to check for . operator and -> operator followed by identifier
// then check for just identifier
std::string Preprocessor::generate_function_body(Function *function) {
    std::string result;
    for (size_t i = 0; i < function->body_tokens.size(); i++) {
        printf("token: %s\n", function->body_tokens[i]->value);
        if (function->body_tokens[i]->type == IDENTIFIER) {
            Function *method_ptr;
            std::string scope, identifier, variable_identifier = function->body_tokens[i]->value;
            // 3 cases
            int type = function->body_tokens[i + 1]->type;
            if ((type == PTR_OP || type == '.') && function->body_tokens[i + 2]->type == IDENTIFIER) {
                identifier = function->body_tokens[i + 2]->value;
                scope = variable_types[variable_identifier];
                std::vector<std::string> arguments;
                std::vector<std::string> argument_types;
                // gather parameters
                if (function->body_tokens[i + 3]->type == '(') {
                    collect_function_arguments(i += 3, function->body_tokens, &arguments, &argument_types);
                    // generate full method identifier and make sure it's valid
                    identifier = unique_function_identifier(scope, identifier, argument_types);
                    if (function_info[identifier]) {
                        result += identifier + (type == PTR_OP ? "(" : "(&") + variable_identifier;
                        for (size_t j = 0; j < arguments.size(); j++) {
                            result += ", " + arguments[j];
                        }
                    } else {
                        error("Invalid method identifier 3");
                    }
                } else { // variable
                    i += 2;
                    result.push_back(' ');
                    result += variable_identifier + (PTR_OP ? "->" : ".") + identifier;
                }
            } else if (variable_types[variable_identifier] == function->scope) {
                result += "self->" + variable_identifier; // this might not always work
            } else if (function->body_tokens[i + 1]->type == '(') {
                std::vector<std::string> arguments;
                std::vector<std::string> argument_types;
                // gather parameters
                collect_function_arguments(i += 1, function->body_tokens, &arguments, &argument_types);

                // generate full method identifier and make sure it's valid
                identifier = unique_function_identifier(scope, identifier, argument_types);
                if (function_info[identifier]) {
                    result += identifier + "(self";
                    for (size_t j = 0; j < arguments.size(); j++) {
                        result += ", " + arguments[j];
                    }
                } else {
                    error("Invalid method identifier");
                }
            } else {
                result += variable_identifier;
            }
        } else if (function->body_tokens[i]->type == SUPER) {
            std::string super = super_class[function->scope];
            int type = function->body_tokens[i + 1]->type;
            if ((type == PTR_OP || type == '.') && function->body_tokens[i + 2]->type == IDENTIFIER) {
                std::string identifier = function->body_tokens[i + 2]->value;
                
            } else {
                error("Error: Invalid use of super");
            }
        } else if (function->body_tokens[i]->type == TYPE_NAME) {
            // case for static members
        } else {
            result += function->body_tokens[i]->value;
            result.push_back(' ');
        }
    }
    return result;
}