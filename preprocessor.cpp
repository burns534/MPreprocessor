#include "preprocessor.h"
#define DEBUG 1
// includes const
static bool is_type_specifier(int token) {
    return token == VOID || token == CHAR
        || token == INT || token == LONG
        || token == FLOAT || token == DOUBLE
        || token == SIGNED || token == UNSIGNED
        || token == TYPE_NAME || token == CONST 
        || token == '*' || token == CLASS_NAME;
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

static std::string unique_class_identifier(std::string identifier) {
    return "_" + identifier;
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
    std::string result, scope, temp, class_definition_string = "#define bool int\n#define true 1\n#define false 0\n";
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
                printf("\n\nit happed with identifier: %s\n", identifier.c_str());
                scope = variable_scope[variable_identifier];
                if (scope.empty()) {
                    result += variable_identifier;
                    continue;
                }
                std::vector<std::string> arguments;
                std::vector<std::string> argument_types;
                // function
                if (new_tokens[cursor + 3]->type == '(') {
                    collect_function_arguments(cursor += 3, new_tokens, &arguments, &argument_types);
                    // generate full method identifier and make sure it's valid
                    identifier = unique_function_identifier(scope, identifier, argument_types);
                    printf("\nidentifier: %s\n\n", identifier.c_str());
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
                    result += identifier + " ";
                }
            } else {
                result += variable_identifier;
            }
        } else if (new_tokens[cursor]->type == CLASS_NAME) { // static
            printf("it was a class name");
            result += unique_class_identifier(new_tokens[cursor]->value);
            result += " ";
            // deal with this in second version
        } else {
            printf("token/type: %s/%d\n", new_tokens[cursor]->value, new_tokens[cursor]->type);
            result += new_tokens[cursor]->value;
            if (new_tokens[cursor]->type != '*')
                result.push_back(' ');
        }
    }
    std::string output = class_definition_string + result;
    // need to make lexer lex a string, not just a file
    FILE *fp = fopen(outfile.c_str(), "w+");
    fwrite(output.c_str(), 1, output.length(), fp);
    fclose(fp);
}

// add inheritance later
std::string Preprocessor::class_declaration() {
    puts("class declaration called");
    bool deinit_found_flag = false;
    if (accept(CLASS) && accept(CLASS_NAME)) {
        #if DEBUG
            puts("inside class declaration");
        #endif
        std::string super_class_identifier, identifier, item_string, function_declarations, class_body = "typedef struct {\n";
        // here the identifier and super identifier are in the original format
        if (accept('{')) {
            identifier = tokens[cursor - 2]->value;
            super_class_identifier = "MObject";
        } else if (accept(':') && accept(CLASS_NAME) && accept('{')) {
            super_class_identifier = tokens[cursor - 2]->value;
            identifier = tokens[cursor - 4]->value;
        } else {
            error("Error: Syntax error");
        }

        
        // identifier = unique_class_identifier(identifier);
        super_class[identifier] = super_class_identifier; // this is correct

        while(1) {
            bool outside_class_body = false;
            // passing original identifier to class item
            if (!(item_string = class_item(identifier, &outside_class_body)).empty()) {
                printf("item string: %s\n", item_string.c_str());
                // if back of string is semicolon, it's a variable dec
                if (outside_class_body) {
                    function_declarations += item_string;
                } else {
                    class_body += item_string;
                }
            } else if (!(item_string = initializer()).empty()) {
                function_declarations += item_string;
            } else if (!deinit_found_flag && !(item_string = deinitializer()).empty()) {
                deinit_found_flag = true;
                function_declarations += item_string;
            } else if (deinit_found_flag && !(item_string = deinitializer()).empty()) {
                error("Error: cannot have multiple deinitializers");
            } else if (accept('}')) {
                return function_declarations + class_body + "}" + unique_class_identifier(identifier) + ";";
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
std::string Preprocessor::class_item(std::string class_name, bool *outside_class_body) {
    #if DEBUG
        puts("class_item");
    #endif
    std::string result;
    std::vector<MToken *> specifiers = declaration_specifiers();
    if (specifiers.size() > 0) {
        // check for return type
        bool contains_return_type = false, is_private = false;
        std::string identifier;
        for (size_t i = 0; i < specifiers.size(); i++) {
            if (!contains_return_type && is_type_specifier(specifiers[i]->type)) {
                contains_return_type = true;
                break;
            } else if (!is_private && specifiers[i]->type == PRIVATE) {
                is_private = true;
            } else if (is_private && specifiers[i]->type == PRIVATE) {
                error("Error: multiple access specifiers");
            } else if (!(*outside_class_body) && specifiers[i]->type == STATIC) {
                puts("STATIC FOUND");
                *outside_class_body = true;
            } else if (*outside_class_body && specifiers[i]->type == STATIC) {
                error("Error: duplicate use of 'static' keyword");
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
            private_identifiers[identifier] = class_name; // original class name
        }

        for(size_t i = 0; i < specifiers.size(); i++) {
            if (specifiers[i]->type != PRIVATE && specifiers[i]->type != STATIC) {
                if (i > 0) result += " ";
                result += std::string(specifiers[i]->value);
                #if DEBUG
                    specifiers[i]->print();
                #endif
            }
        }
        // distinguish between variable dec, variable def, or function def
        if (accept(';')) { // variable declaration
            result += " " + identifier;
            variable_scope[identifier] = class_name;
            printf("variable: scope -> %s: %s\n", identifier.c_str(), class_name.c_str());
            return result + ";\n";
        } else if (accept(',')) {
            result += " " + identifier;
            variable_scope[identifier] = class_name;
            printf("variable: scope -> %s: %s\n", identifier.c_str(), class_name.c_str());
            for (;cursor < tokenCount; cursor++) {
                if (accept(';')) {
                    break;
                } else if (tokens[cursor]->type == '*') {
                    assert(tokens[++cursor]->type == IDENTIFIER);
                    variable_scope[tokens[cursor]->value] = class_name;
                    result += ", *";
                    result += tokens[cursor]->value;
                } else if (tokens[cursor]->type == IDENTIFIER) {
                    variable_scope[tokens[cursor]->value] = class_name;
                    result += ", ";
                    result += tokens[cursor]->value;
                } else if (tokens[cursor]->type == ',') {
                    continue;
                } else {
                    error("Invalid token in variable declaration list");
                }
            }
            return result + ";\n";
        } else if (accept('(')) { // function definition
            std::vector<Parameter> params = parameter_list();
            std::string method_name, parameter_string = unique_class_identifier(class_name) + " *self";
            for (size_t i = 0; i < params.size(); i++) {
                parameter_string += ", ";
                for (size_t j = 0; j < params[i].specifiers.size(); j++) {
                    if (j > 0) parameter_string += " ";
                    if (params[i].specifiers[i]->type == CLASS_NAME) {
                        parameter_string += unique_class_identifier(params[i].specifiers[j]->value);
                    } else {
                        parameter_string += params[i].specifiers[j]->value;
                    }
                }
                parameter_string += params[i].name;
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
                // original scope here as well
                method_name = unique_function_identifier(class_name, identifier, params);
                printf("method_name: %s\n", method_name.c_str());
                result += method_name + " ";
                // original scope
                Function *function = new Function(class_name, specifiers, identifier, params, body_tokens);
                function_info[method_name] = function;
                result += "(" + parameter_string + ") {" + generate_function_body(function) + "}\n";
                *outside_class_body = true;
                return result;
            } else {
                error("Error: Syntax error in function definition");
            }
        } else if (accept('=')) {
            // push to a queue for initialization inside initializer then return result
//TODO: skip for now. Will be implemented in version 2
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
        printf("decspec loop token: %s\n", tokens[cursor]->value);
        if (accept(TYPEDEF)) {
            // skip typedefs completely
            while(cursor < tokenCount && tokens[cursor++]->type != ';');
            // printf("\n\n\n\n\n HERE %s \n\n\n\n", tokens[cursor]->value);
            // return result;
        } else if ((token = type_qualifier())) {
            result.push_back(token);
        } else if ((token = storage_class_specifier())) {
            result.push_back(token);
        } else if ((token = type_specifier())) {
            result.push_back(token);
        } else if (accept('*')) {
            result.push_back(tokens[cursor - 1]);
        } else {
            if (!result.empty()) {
                puts("\n\n\tDECLARATION SPECIFIERS FOUND\n\n");
                for (size_t i = 0; i < result.size(); i++)
                    printf("%s ", result[i]->value);
                printf("\n");
            }
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
    || accept(UNSIGNED) || accept(TYPE_NAME)
    || accept(CLASS_NAME)) {
        return tokens[cursor - 1];
    } else {
        printf("returning nullptr for token: %s\n", tokens[cursor]->value);
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
    if (accept(EXTERN) || accept(STATIC) || accept(AUTO) || accept(REGISTER)) {
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

// anonymous classes or class typedefs are not allowed
// replace IDENTIFIER with TYPE_NAME for user defined types
void Preprocessor::first_pass() {
#if DEBUG
    puts("first_pass called");
#endif
    std::map<std::string, int> type_names;
    for (size_t i = 0; i < tokenCount; i++) {
        int token = tokens[i]->type;
        printf("token: %d\n", token);
        if (token == CLASS) {
            assert(tokens[++i]->type == IDENTIFIER);
            tokens[i]->type = CLASS_NAME;
            type_names[tokens[i]->value] = CLASS_NAME;
        } else if (token == STRUCT || token == UNION || token == ENUM) {
            assert(tokens[++i]->type == IDENTIFIER);
            tokens[i]->type = TYPE_NAME;
            type_names[tokens[i]->value] = TYPE_NAME;
        } else if (tokens[i]->type == TYPEDEF) {
            token = tokens[++i]->type;
            if (token == STRUCT || token == UNION || token == ENUM) {
                assert(tokens[++i]->type == '{');
                int open_count = 1;
                while(open_count > 0) {
                    if (tokens[++i]->type == '{') {
                        open_count++;
                    } else if (tokens[i]->type == '}') {
                        open_count--;
                    }
                }
                assert(tokens[++i]->type == IDENTIFIER);
                tokens[i]->type = TYPE_NAME;
                type_names[tokens[i]->value] = TYPE_NAME;
                // declaration list
                while(1) {
                    if (tokens[++i]->type == ',') {
                        assert(tokens[++i]->type == IDENTIFIER);
                        tokens[i]->type = TYPE_NAME;
                        type_names[tokens[i]->value] = TYPE_NAME;
                    } else {
                        break;
                    }
                }
            } else {
                while (tokens[++i]->type != ';');
                tokens[i - 1]->type = TYPE_NAME;
                type_names[tokens[i - 1]->value] = TYPE_NAME;
                printf("just inserted identifier %s\n", tokens[i]->value);
            }
        } else if (tokens[i]->type == IDENTIFIER && type_names[tokens[i]->value]) {
            tokens[i]->type = type_names[tokens[i]->value];
        } else {
            printf("rejecting identifier/type: %s/%d\n", tokens[i]->value, tokens[i]->type);
        }
    }

    for (size_t i = 0; i < tokenCount; i++) {
        if (tokens[i]->type == TYPE_NAME) {
            printf("typename: %s\n", tokens[i]->value);
        }
    }
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
            if (accept(';') || accept('=') || accept(',') || accept(')')) {
                if (specifiers.size() > 0) {
                    std::string type;
                    for (size_t i = 0; i < specifiers.size(); i++) {
                        if (is_type_specifier(specifiers[i]->type)) {
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
    printf("\n\n\tGATHERED VARIABLE TYPES\n\nvariable: type\n");
    for (std::map<std::string, std::string>::iterator it = variable_types.begin(); it != variable_types.end(); it++) {
        printf("%s : %s\n\n", it->first.c_str(), it->second.c_str());
    }
    cursor = 0;
}

static bool is_float(const char *string) {
    for(size_t i = 0; i < strlen(string); i++) {
        if (string[i] == '.') return true;
    }
    return false;
}
// only supports double and int parameter types..
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
            // determine type of token
            argument_types->push_back(is_float(tokens[i]->value) ? "double" : "int");
        }
    }
}
std::string Preprocessor::generate_function_body(Function *function) {
    printf("\n\n\tCALL\n\ngenerate_function_body for %s\n", function->identifier.c_str());
    std::string result;
    MToken *token = function->body_tokens[0];
    for (size_t i = 0; i < function->body_tokens.size(); token = function->body_tokens[++i]) {
        printf("token/type: %s/%d\n", token->value, token->type);
        if (token->type == IDENTIFIER) {
            Function *method_ptr;
            std::string scope, identifier, variable_identifier = token->value;
            // 3 cases
            int type = function->body_tokens[i + 1]->type;
            if ((type == PTR_OP || type == '.') && function->body_tokens[i + 2]->type == IDENTIFIER) {
                identifier = function->body_tokens[i + 2]->value;
                // original class name
                scope = variable_scope[variable_identifier];
                std::vector<std::string> arguments;
                std::vector<std::string> argument_types;
                // gather parameters
                if (function->body_tokens[i + 3]->type == '(') {
                    collect_function_arguments(i += 3, function->body_tokens, &arguments, &argument_types);
                    // generate full method identifier and make sure it's valid
                    // original class name
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
                    result += variable_identifier + (PTR_OP ? "->" : ".") + identifier + " ";
                }
                // good
            } else if (variable_scope[variable_identifier] == function->scope) {
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
                    result += ")";
                } else {
                    error("Invalid method identifier");
                }
            } else {
                // spacing just to be safe...
                printf("variable_identifier: %s\n", variable_identifier.c_str());
                result += variable_identifier + " ";
            }
            // this isn't recursive so it won't allow super.super for now. Need to fix later
        } else if (token->type == SUPER) {
            std::string super = super_class[function->scope];
            int type = function->body_tokens[i + 1]->type;
            if ((type == PTR_OP || type == '.') && function->body_tokens[i + 2]->type == IDENTIFIER) {
                std::string identifier = function->body_tokens[i + 2]->value;
                if (function->body_tokens[i + 3]->type == '(') {
                    std::vector<std::string> arguments;
                    std::vector<std::string> argument_types;
                    collect_function_arguments(i += 3, function->body_tokens, &arguments, &argument_types);
                    identifier = unique_function_identifier(super, identifier, argument_types);
                    if (function_info[identifier]) {
                        // implicit C cast from self* to super*
                        result += identifier + "(self";
                        for (size_t j = 0; j < arguments.size(); j++) {
                            result += ", " + arguments[j];
                        }
                        result += ")";
                    } else {
                        error("Invalid method identifier");
                    }
                } else { // variable
                    i += 2;
                    result += super + (PTR_OP ? "->" : ".") + identifier + " ";
                }
            } else {
                error("Error: Invalid use of super");
            }
        } else if (token->type == CLASS_NAME) {
            result += unique_class_identifier(token->value);
            result += " ";
        } else {
            result += token->value;
            if (token->type != '*')
                result.push_back(' ');
        }
    }
    return result;
}