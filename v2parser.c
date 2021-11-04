#include "v2parser.h"

#define CURSOR_STACK_DEPTH 4096
// #define CHILD_BUFFER_SIZE 4096
#define ARGUMENT_LIST_MAX 32
#define DEFAULT_CHILD_SIZE 8

static Token **tokens;
static size_t tc, cur = 0;
static map *primitive_types, *type_names;
static size_t cursor_stack[4096], cursor_stack_top = 0;

static void error(char *fmt, ...) {
    
    int ret;

    size_t len = strlen(fmt);
    char buffer[500], *message;

    sprintf(buffer, "%s:%lu:%lu: \x1b[31;1merror\x1b[0m : ", tokens[cur]->filename, tokens[cur]->line_number, tokens[cur]->character);

    /* Declare a va_list type variable */
    va_list myargs;

    /* Initialise the va_list variable with the ... after fmt */

    va_start(myargs, fmt);

    ret = vsprintf(buffer + strlen(buffer), fmt, myargs);

    /* Clean up the va_list */
    va_end(myargs);

    message = malloc(strlen(buffer) + 1);
    strcpy(message, buffer);

    perror(message);

    exit(EXIT_FAILURE);
}

static inline int pop() {
    if (cursor_stack_top > 0)
        cursor_stack_top--;
    return 1;
}

static inline int pop_set() {
    if (cursor_stack_top > 0)
        cur = cursor_stack[--cursor_stack_top];
    return 1;
}

static inline int push() {
    if (cursor_stack_top + 1 < CURSOR_STACK_DEPTH)
        cursor_stack[cursor_stack_top++] = cur;
    else
        perror("stack overflow");
    return 1;
}

static void print_node(ASTNode *node) {
    if (!node) return;
    printf("%s: %s", node_type_to_string(node->type), node->value);
}

static bool add_child(ASTNode *c, ASTNode *p) {
    if (!c || !p) return false;
    if (p->child_count > 0.6 * p->children_size)
        p->children = realloc(p->children, p->children_size *= 2);
    p->children[p->child_count++] = c;
    printf("added child ");
    print_node(c);
    printf(" to ");
    print_node(p);
    printf("\n");
    return true;
}

static void pop_child(ASTNode *p) {
    if (p->child_count > 0)
        p->child_count--;
}

static ASTNode * create_node(ASTNodeType type) {
    ASTNode *result = malloc(sizeof(ASTNode));
    result->child_count = 0;
    result->children_size = DEFAULT_CHILD_SIZE;
    result->type = type;
    result->children = malloc(sizeof(ASTNode *) * DEFAULT_CHILD_SIZE);
    return result;
}

static ASTNode * create_node_with_child(ASTNodeType type, ASTNode *child) {
    ASTNode *result = malloc(sizeof(ASTNode));
    result->child_count = 1;
    result->children_size = DEFAULT_CHILD_SIZE;
    result->type = type;
    result->children = malloc(sizeof(ASTNode *) * DEFAULT_CHILD_SIZE);
    *result->children = child;
    return result;
}

static ASTNode * create_node_with_value(ASTNodeType type, char *v) {
    ASTNode *result = malloc(sizeof(ASTNode));
    result->child_count = 0;
    result->children_size = DEFAULT_CHILD_SIZE;
    result->type = type;
    result->value = v;
    result->children = malloc(sizeof(ASTNode *) * DEFAULT_CHILD_SIZE);
    return result;
}

static inline void print_tabs(int count) {
    for (int i = 0; i < count; i++)
        putchar('\t');
}

static inline void print_space(int count, int num) {
    for (int i = 0; i < count * num; i++)
        putchar(' ');
}

static void print_tree_util(ASTNode *node, int level) {
    if (node) {
        // print_tabs(level);
        print_space(level, 2);
        printf("%s: %s\n", node_type_to_string(node->type), node->value);

        if (node->child_count) {
            // print_tabs(level);
            print_space(level, 2);
            putchar('\r');
        }
        for (int i = 0; i < node->child_count; i++) 
            print_tree_util(node->children[i], level + 1);
    }
}

void print_tree(ASTNode *root) {
    print_tree_util(root, 0);
}

#define PRIMITIVE_TYPES 9

static char *primitive_types_array[PRIMITIVE_TYPES] = {
    "bool",
    "ulong",
    "uint",
    "double",
    "float",
    "long",
    "char",
    "void",
    "int"
};

// for primitives
static inline bool primitive_type() {
    return tokens[cur]->type == IDENTIFIER && map_contains(tokens[cur]->value, primitive_types);
}

// seems like I'm losing useful type information here that I need later...
static ASTNode * numeric_literal() {
    if (cur >= tc) return NULL;
    printf("numeric_literal %s\n", tokens[cur]->value);
    if (tokens[cur]->type == INTEGER_LITERAL || tokens[cur]->type == FLOATING_POINT_LITERAL) 
        return create_node_with_value(NUMERIC_LITERAL, tokens[cur++]->value);
    if (push() && tokens[cur++]->subtype == '-') {
        // cases 1 and 3
        if ((tokens[cur]->type == INTEGER_LITERAL || tokens[cur]->type == FLOATING_POINT_LITERAL) && pop()) {
            ASTNode *result = create_node(NUMERIC_LITERAL);
            char *buffer = malloc(strlen(tokens[cur]->value) + 2);
            buffer[0] = '-';
            strcat(buffer + 1, tokens[cur]->value);
            result->value = buffer;
            return result;
        }
    } 
    pop_set();
    return NULL;
}

static inline ASTNode * boolean_literal() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == TRUE || tokens[cur]->type == FALSE) 
        return create_node_with_value(BOOLEAN_LITERAL, tokens[cur++]->value);
    return NULL;
}

static inline ASTNode * nil_literal() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == NIL)
        return create_node_with_value(NIL_LITERAL, tokens[cur++]->value);
    return NULL;
}

static inline ASTNode * string_literal() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == STATIC_STRING_LITERAL || tokens[cur]->type == INTERPOLATED_STRING_LITERAL) 
        return create_node_with_value(STRING_LITERAL, tokens[cur++]->value);
    return NULL;
}

static ASTNode * literal() {
    puts("literal");
    ASTNode *c;
    if ((c = numeric_literal())
    || (c = string_literal())
    || (c = boolean_literal())
    || (c = nil_literal())) 
        return create_node_with_child(LITERAL, c);
    return NULL;
}

static ASTNode * access_level_modifier() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == PRIVATE || tokens[cur]->type == PUBLIC)
        return create_node_with_value(ACCESS_LEVEL_MODIFIER, tokens[cur++]->value);
    return NULL;
}
// operators are passed through from lexer with their whitespace intact
static ASTNode * binary_operator() {
    if (cur >= tc) return NULL;
    printf("binary operator called with token %s and type %d\n", tokens[cur]->value, tokens[cur]->fix);
    if (tokens[cur]->type == OPERATOR && tokens[cur]->fix == INF) 
        return create_node_with_value(BINARY_OPERATOR, tokens[cur++]->value);
    return NULL;
}

static ASTNode * prefix_operator() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == OPERATOR && tokens[cur]->fix == PRE) 
        return create_node_with_value(PREFIX_OPERATOR, tokens[cur++]->value);
    return NULL;
}

static ASTNode * postfix_operator() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == OPERATOR && tokens[cur]->fix == POST) 
        return create_node_with_value(POSTFIX_OPERATOR, tokens[cur++]->value);
    return NULL;
}

static ASTNode * type_name() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == IDENTIFIER && (primitive_type() || map_contains(tokens[cur]->value, type_names)))
        return create_node_with_value(TYPE_NAME, tokens[cur++]->value);
    return NULL;
}

static ASTNode * generic_argument() {
    ASTNode *c;
    if ((c = type()))
        return create_node_with_child(GENERIC_ARGUMENT, c);
    return NULL;
}

static ASTNode * generic_argument_list() {
    ASTNode *c;
    if ((c = generic_argument())) {
        ASTNode *result = create_node_with_child(GENERIC_ARGUMENT_LIST, c);
        while (cur < tc && tokens[cur++]->type == COMMA) 
            if (!add_child(generic_argument(), result))
                error("expected type in generic argument list");
        return result;
    }
    return NULL;
}

// can't be empty
// operator stores < and >
static ASTNode * generic_argument_clause() {
    ASTNode *c;
    if (cur < tc && push() 
    && tokens[cur++]->subtype == '<'
    && (c = generic_argument_list())
    && cur < tc && tokens[cur++]->subtype == '>' && pop()) {
        return create_node_with_child(GENERIC_ARGUMENT_CLAUSE, c);
    }
    pop_set();
    return NULL;
}

static ASTNode * type_identifier() {
    ASTNode *c;
    if ((c = type_name())) {
        ASTNode *result = create_node_with_child(TYPE_IDENTIFIER, c);
        if (add_child(generic_argument_clause(), result)) {
            if (cur < tc && tokens[cur]->subtype == '.') 
                if (++cur && !add_child(type_identifier(), result))
                    error("expected type identifier following dot");
        } else if (cur < tc && tokens[cur]->subtype == '.') 
            if (++cur && !add_child(type_identifier(), result))
                error("expected type identifier following .");
        
        return result;
    }
    return NULL;
}

static ASTNode * type_annotation() {
    if (cur >= tc) return NULL;
    if (push() && tokens[cur++]->type == COLON) {
        ASTNode *result = create_node(TYPE_ANNOTATION);
        if (!add_child(type(), result))
            error("expected type following :");
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * element_name() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == IDENTIFIER)
        return create_node_with_value(ELEMENT_NAME, tokens[cur++]->value);
    return NULL;
}

static ASTNode * tuple_type_element() {
    ASTNode *c;
    if ((c = type()))
        return create_node_with_child(TUPLE_TYPE_ELEMENT, c);
    else if ((c = element_name())) {
        ASTNode *result = create_node_with_child(TUPLE_TYPE_ELEMENT, c);
        if (add_child(type_annotation(), result))
            return result;
        free(c);
        free(result);
    } 
    return NULL;
}

static ASTNode * tuple_type_element_list() {
    ASTNode *c;
    if ((c = tuple_type_element())) {
        ASTNode *result = create_node_with_child(TUPLE_TYPE_ELEMENT_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(tuple_type_element(), result))
                error("expected type element following comma");
        return result;
    }
    return NULL;
}

static ASTNode * tuple_type() {
    if (cur >= tc) return NULL;
    if (push() && tokens[cur++]->type == OPEN_PAREN) {
        if (cur < tc && tokens[cur]->type == CLOSE_PAREN && ++cur)
            return create_node(TUPLE_TYPE);
        ASTNode *c;
        if ((c = tuple_type_element())) {
            if (cur < tc && tokens[cur++]->type != COMMA)
                error("expected comma in tuple type");
            ASTNode *result = create_node_with_child(TUPLE_TYPE, c);
            if (!add_child(tuple_type_element_list(), result))
                error("expected tuple type element list following comma");
            return result;
        }
    }
    return NULL;
}

static ASTNode * argument_label() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == IDENTIFIER)
        return create_node_with_value(ARGUMENT_LABEL, tokens[cur++]->value);
    return NULL;
}

static ASTNode * function_type_argument() {
    ASTNode *c;
    if ((c = type())) 
        return create_node_with_child(FUNCTION_TYPE_ARGUMENT, c);
    if ((c = argument_label())) {
        ASTNode *result = create_node_with_child(FUNCTION_TYPE_ARGUMENT, c);
        if (!add_child(type_annotation(), result))
            error("expected type annotation following argument label");
        return result;
    }
    return NULL;
}

static ASTNode * function_type_argument_list() {
    ASTNode *c;
    if ((c = function_type_argument())) {
        ASTNode *result = create_node_with_child(FUNCTION_TYPE_ARGUMENT_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(function_type_argument(), result))
                error("expected type element following comma");
        return result;
    }
    return NULL;
}

static ASTNode * function_type_argument_clause() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == OPEN_PAREN) {
        if (cur < tc && tokens[++cur]->type == CLOSE_PAREN && ++cur)
            return create_node(FUNCTION_TYPE_ARGUMENT_CLAUSE);
        ASTNode *c;
        if ((c = function_type_argument_list())) {
            ASTNode *result = create_node_with_child(FUNCTION_TYPE_ARGUMENT_CLAUSE, c);
            // variadic
            if (cur < tc && tokens[cur]->subtype == '*')
                result->value = tokens[cur++]->value;
            
            if (cur < tc && tokens[cur++]->type != CLOSE_PAREN)
                error("expected )");

            return result;
        }
    }
    return NULL;
}

static ASTNode * function_type() {
    ASTNode *c;
    if ((c = function_type_argument_clause())) {
        ASTNode *result = create_node_with_child(FUNCTION_TYPE, c);
        if (cur < tc && tokens[cur]->type == THROWS && cur++)
            result->value = "throws";
        if (cur + 1 >= tc)
            error("eof during function type");
        if (tokens[cur++]->type != RETURN_ANNOTATION)
            error("expected return annotation");
        if (!add_child(type(), result))
            error("expected return type");
        return result;
    }
    return NULL;
}

static ASTNode * array_type() {
    ASTNode *c;
    if (cur < tc && push() && tokens[cur++]->type == OPEN_SQ_BRACE && (c = type()) && cur < tc && tokens[cur++]->type == CLOSE_SQ_BRACE && pop())
        return create_node_with_child(ARRAY_TYPE, c);
    pop_set();
    return NULL;
}

static ASTNode * dictionary_type() {
    ASTNode *c;
    if (cur < tc && push() && tokens[cur++]->type == OPEN_SQ_BRACE && (c = type()) && cur < tc && tokens[cur++]->type == COLON && pop()) {
        ASTNode *result = create_node_with_child(DICTIONARY_TYPE, c);
        if (!add_child(type(), result))
            error("expected type after colon");
        if (cur >= tc)
            error("eof in dictioary type");
        if (tokens[cur++]->type != CLOSE_SQ_BRACE)
            error("expected close brace after type");
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * optional_type() {
    ASTNode *c;
    if (push() && (c = type()) && cur < tc 
    && tokens[cur++]->subtype == '!' && pop()) 
        return create_node_with_child(OPTIONAL_TYPE, c);
    pop_set();
    return NULL;    
}

static ASTNode * implicitly_unwrapped_optional_type() {
    ASTNode *c;
    if (push() && (c = type()) && cur < tc 
    && tokens[cur++]->subtype == '?' && pop()) 
        return create_node_with_child(IMPLICITLY_UNWRAPPED_OPTIONAL_TYPE, c);
    pop_set();
    return NULL;    
}


static ASTNode * protocol_composition_type() {
    ASTNode *c;
    if (push() && (c = type_identifier())) {
        ASTNode *result = create_node_with_child(PROTOCOL_COMPOSITION_TYPE, c);
        // must have at least one
        while (cur < tc && tokens[cur]->subtype == '&' && cur++)
            if (!add_child(type_identifier(), result))
                error("expected type identifier following &");
        
        if (result->child_count > 0 && pop())
            return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * any_type() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == ANY && ++cur) 
        return create_node(ANY_TYPE);
    return NULL;
}

static ASTNode * type_inheritance_list() {
    ASTNode *c;
    if ((c = type_identifier())) {
        ASTNode *result = create_node_with_child(TYPE_INHERITANCE_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(type_identifier(), result))
                error("expected type identifier following comma");
        return result;
    }
    return NULL;
}

// dont need push because errors
static ASTNode * type_inheritance_clause() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == COLON && cur++) {
        ASTNode *result = create_node(TYPE_INHERITANCE_CLAUSE);
        if (!add_child(type_inheritance_list(), result))
            error("expected type inheritance list following colon");
        return result;
    }
    return NULL;
}

ASTNode * type() {
    ASTNode *c;
    if ((c = function_type())
    || (c = array_type())
    || (c = dictionary_type())
    || (c = type_identifier())
    || (c = tuple_type())
    || (c = optional_type())
    || (c = implicitly_unwrapped_optional_type())
    || (c = protocol_composition_type())
    || (c = any_type()))
        return create_node_with_child(TYPE, c);
    if (cur < tc && push() && tokens[cur++]->type == OPEN_PAREN
    && (c = type()) && cur < tc && tokens[cur++]->type == CLOSE_PAREN && pop()) {
        ASTNode *result = create_node_with_child(TYPE, c);
        result->value = "parenthesized";
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * declaration_modifier() {
    if (cur >= tc) return NULL;
    switch(tokens[cur]->type) {
        case CLASS:
        case CONVENIENCE:
        case FINAL:
        case INFIX:
        case LAZY:
        case OPTIONAL:
        case OVERRIDE:
        case POSTFIX:
        case PREFIX:
        case REQUIRED:
        case STATIC:
        case UNOWNED:
        case WEAK:
            return create_node_with_value(DECLARATION_MODIFIER, tokens[cur++]->value);
            break;
        default:
            break;
    }

    ASTNode *c;
    if ((c = access_level_modifier()))
        return create_node_with_child(DECLARATION_MODIFIER, c);
    return NULL;
}

static ASTNode * declaration_modifiers() {
    ASTNode *c;
    if ((c = declaration_modifier())) {
        ASTNode *result = create_node_with_child(DECLARATION_MODIFIERS, c);
        while(add_child(declaration_modifier(), result))
        return result;
    }
    return NULL;
}

static ASTNode * try_operator() {
    printf("try operator with cur: %lu\n", cur);
    if (cur < tc && tokens[cur]->type == TRY && cur++) {
        ASTNode *result = create_node(TRY_OPERATOR);
        if (cur < tc && (tokens[cur]->subtype == '?' || tokens[cur]->subtype == '!')) 
            result->value = tokens[cur++]->value;
        return result;
    }
    return NULL;
}

static ASTNode * array_literal_item() {
    ASTNode *c;
    if ((c = expression())) 
        return create_node_with_child(ARRAY_LITERAL_ITEM, c);
    return NULL;
}

static ASTNode * array_literal_items() {
    ASTNode *c;
    if ((c = array_literal_item())) {
        ASTNode *result = create_node_with_child(ARRAY_LITERAL_ITEMS, c);
        while(cur < tc && tokens[cur]->type == COMMA && cur++) {
            if (!add_child(array_literal_item(), result))
                error("expected array literal item following comma");
        }
        return result;
    }
    return NULL;
}

static ASTNode * array_literal() {
    if (cur >= tc) return NULL;
    if (push() && tokens[cur++]->type == OPEN_SQ_BRACE) {
        if (cur < tc && tokens[cur]->type == CLOSE_SQ_BRACE && cur++ && pop())
            return create_node(ARRAY_LITERAL);
        ASTNode *c;
        if ((c = array_literal_items())) {
            ASTNode *result = create_node_with_child(ARRAY_LITERAL, c);
            if (cur >= tc) return NULL;
            if (pop() && tokens[cur++]->type != CLOSE_SQ_BRACE)
                error("expected ] following array literal litems");
            return result;
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * dictionary_literal_item() {
    ASTNode *c;
    if ((c = expression()) && cur < tc && tokens[cur]->type == COLON && cur++) {
        ASTNode *result = create_node_with_child(DICTIONARY_LITERAL_ITEM, c);
        if (!add_child(expression(), result))
            error("expected expression following colon");
        return result;
    }
    return NULL;
}

static ASTNode * dictionary_literal_items() {
    ASTNode *c;
    if ((c = dictionary_literal_item())) {
        ASTNode *result = create_node_with_child(DICTIONARY_LITERAL_ITEMS, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (cur++ && !add_child(dictionary_literal_item(), result))
                error("expected dictionary literal item following comma");
        return result;
    }
    return NULL;
}

static ASTNode * dictionary_literal() {
    if (cur >= tc) return NULL;
    if (push() && tokens[cur++]->type == OPEN_SQ_BRACE) {
        if (cur < tc && tokens[cur]->type == COLON && cur++) {
            if (cur >= tc) return NULL;
            if (pop() && tokens[cur++]->type != CLOSE_SQ_BRACE)
                error("expected ] following colon in dictionary literal");
            return create_node(ARRAY_LITERAL);
        }
        ASTNode *c;
        if ((c = dictionary_literal_items())) {
            if (cur >= tc) return NULL;
            if (pop() && tokens[cur++]->type != CLOSE_SQ_BRACE)
                error("expected ] following dictionary literal litems");
            return create_node_with_child(ARRAY_LITERAL, c);
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * literal_expression() {
    puts("literal_expression");
    ASTNode *c;
    if ((c = literal())
    || (c = array_literal()) 
    || (c = dictionary_literal())) 
        return create_node_with_child(LITERAL_EXPRESSION, c);
    return NULL;
}

static ASTNode * self_initializer_expression() {
    if (cur >= tc + 2) return NULL;
    if (push() && tokens[cur++]->type == SELF 
    && tokens[cur++]->subtype == '.'
    && tokens[cur++]->type == INIT && pop())
        return create_node(SELF_INITIALIZER_EXPRESSION);
    pop_set();
    return NULL;
}

static ASTNode * self_subscript_expression() {
    return NULL;
}

static ASTNode * self_method_expression() {
    if (cur >= tc + 2) return NULL;
    if (push() && tokens[cur++]->type == SELF 
    && tokens[cur++]->subtype == '.'
    && tokens[cur++]->type == IDENTIFIER && pop())
        return create_node(SELF_METHOD_EXPRESSION);
    pop_set();
    return NULL;
}

static ASTNode * self_expression() {
    ASTNode *c;
    if ((c = self_method_expression())
    || (c = self_subscript_expression())
    || (c = self_initializer_expression())) 
        return create_node_with_child(SELF_EXPRESSION, c);
    
    if (cur < tc && tokens[cur]->type == SELF && cur++)
        return create_node(SELF_EXPRESSION);
    return NULL;
}

static ASTNode * superclass_initializer_expression() {
    if (cur >= tc + 2) return NULL;
    if (push() && tokens[cur++]->type == SUPER 
    && tokens[cur++]->subtype == '.'
    && tokens[cur++]->type == INIT && pop())
        return create_node(SUPERCLASS_INITIALIZER_EXPRESSION);
    pop_set();
    return NULL;
}

static ASTNode * superclass_subscript_expression() {
    return NULL;
}

static ASTNode * superclass_method_expression() {
    if (cur >= tc + 2) return NULL;
    if (push() && tokens[cur++]->type == SUPER 
    && tokens[cur++]->subtype == '.'
    && tokens[cur++]->type == IDENTIFIER && pop())
        return create_node(SUPERCLASS_METHOD_EXPRESSION);
    pop_set();
    return NULL;
}

static ASTNode * superclass_expression() {
    ASTNode *c;
    if ((c = superclass_method_expression())
    || (c = superclass_subscript_expression())
    || (c = superclass_initializer_expression())) 
        return create_node_with_child(SUPERCLASS_EXPRESSION, c);
    
    if (cur < tc && tokens[cur]->type == SUPER && cur++)
        return create_node(SUPERCLASS_EXPRESSION);
    return NULL;
}

// closures

static ASTNode * closure_parameter_name() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == IDENTIFIER) 
        return create_node_with_value(CLOSURE_PARAMETER_NAME, tokens[cur++]->value);
    return NULL;
}
// should really have subtypes for operator token or something better
static ASTNode * closure_parameter() {
    ASTNode *c;
    if ((c = closure_parameter_name())) {
        ASTNode *result = create_node_with_child(CLOSURE_PARAMETER, c);
        add_child(type_annotation(), result);
        if (cur < tc && tokens[cur]->subtype == '.') {
            // variadic
            result->value = tokens[cur++]->value;
            return result;
        }
        return result;
    }
    return NULL;
}

static ASTNode * closure_parameter_list() {
    ASTNode *c;
    if ((c = closure_parameter())) {
        ASTNode *result = create_node_with_child(CLOSURE_PARAMETER_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(closure_parameter(), result))
                error("expected closure parameter following comma");
        return result;
    }
    return NULL;
}

// static ASTNode * identifier_list() {
//     ASTNode *c;
//     if ((c = identifier())) {
//         ASTNode *result = create_node_with_child(IDENTIFIER_LIST, c);
//         while(cur < tc && tokens[cur]->type == COMMA) {
//             if (cur++ && !add_child(identifier(), result))
//                 error("expected identifier following comma");
//         }
//         return result;
//     }
//     return NULL;
// }

static ASTNode * closure_parameter_clause() {
    if (cur >= tc) return NULL;
    if (push() && tokens[cur++]->type == OPEN_PAREN) {
        if (cur < tc && tokens[cur]->type == CLOSE_PAREN && cur++ && pop()) 
            return create_node(CLOSURE_PARAMETER_CLAUSE);
        ASTNode *result = create_node(CLOSURE_PARAMETER_CLAUSE);
        if (add_child(closure_parameter_list(), result)) {
            if (cur >= tc) return NULL;
            if (pop() && tokens[cur++]->type != CLOSE_PAREN)
                error("expected ) following closure parameter list");
        }
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * function_result() {
    ASTNode *c;
    if ((c = type())) 
        return create_node_with_child(FUNCTION_RESULT, c);
    return NULL;
}

static ASTNode * closure_signature() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == IN && cur++)
        return create_node(CLOSURE_SIGNATURE); 
    ASTNode *c;
    if ((c = closure_parameter_clause())) {
        ASTNode *result = create_node_with_child(CLOSURE_SIGNATURE, c);
        if (cur < tc && tokens[cur]->type == THROWS && cur++)
            result->value = "throws";
        add_child(function_result(), result); // optional
        if (cur >= tc) return NULL;
        if (tokens[cur]->type != IN) 
            error("expected keyword \"in\"");
        return result;
    }
    return NULL;
}

static ASTNode * closure_expression() {
    if (cur >= tc) return NULL;
    if (push() && tokens[cur++]->type == OPEN_CURL_BRACE) {
        if (cur < tc && tokens[cur]->type == CLOSE_CURL_BRACE && cur++ && pop()) 
            return create_node(CLOSURE_EXPRESSION);
        ASTNode *result = create_node(CLOSURE_EXPRESSION);
        add_child(closure_signature(), result);
        add_child(statements(), result);
        // must have at least one of the child types
        if (result->child_count > 0 && pop())
            return result;
    }
    pop_set();
    return NULL;
}

// other expressions

static ASTNode * parenthesized_expression() {
    if (cur >= tc) return NULL;
    ASTNode *c;
    if (push() && tokens[cur++]->type == OPEN_PAREN && ((c = expression()) && cur < tc && tokens[cur++]->type == CLOSE_PAREN) && pop())
        return create_node_with_child(PARENTHESIZED_EXPRESSION, c);
    pop_set();
    return NULL;
}

static ASTNode * tuple_element() {
    if (cur >= tc) return NULL;
    ASTNode *c;
    if ((c = expression()))
        return create_node_with_child(TUPLE_ELEMENT, c);
    if (push() && tokens[cur]->type == IDENTIFIER) {
        ASTNode *result = create_node_with_value(TUPLE_ELEMENT, tokens[cur++]->value);
        if (cur < tc && tokens[cur++]->type == COLON)
            if (pop() && !add_child(expression(), result))
                error("expected expression following colon");
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * tuple_element_list() {
    ASTNode *c;
    if ((c = tuple_element())) {
        ASTNode *result = create_node_with_child(TUPLE_ELEMENT_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(tuple_element(), result))
                error("expected tuple element following comma");
        return result;
    }
    return NULL;
}

static ASTNode * tuple_expression() {
    if (cur >= tc) return NULL;
    if (push() && tokens[cur++]->type == OPEN_PAREN) {
        if (cur < tc && tokens[cur]->type == CLOSE_PAREN && cur++ && pop())
            return create_node(TUPLE_EXPRESSION);
        ASTNode *c;
        if ((c = tuple_element_list())) {
            if (cur >= tc) return NULL;
            if (pop() && tokens[cur++]->type != CLOSE_PAREN)
                error("expected ) following tuple element list");
            return create_node_with_child(TUPLE_EXPRESSION, c);
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * wildcard_expression() {
    if (cur >= tc) return NULL;
    if (tokens[cur]->type == WILDCARD) 
        return create_node_with_value(WILDCARD_EXPRESSION, tokens[cur++]->value);
    return NULL;
}
// missing syntax check
static ASTNode * selector_expression() {
    if (cur >= tc + 2) return NULL;
    ASTNode *c;
    if (push() && tokens[cur++]->type == HASHTAG
    &&  tokens[cur++]->type == SELECTOR 
    && tokens[cur++]->type == OPEN_PAREN
    && (c = expression()) && cur < tc && tokens[cur++]->type == CLOSE_PAREN && pop())
        return create_node_with_child(SELECTOR_EXPRESSION, c);
    pop_set();
    return NULL;
}

static ASTNode * primary_expression() {
    if (cur >= tc) return NULL;
    printf("primary_expression %s\n", tokens[cur]->value);
    ASTNode *c;
    if (tokens[cur]->type == IDENTIFIER) {
        ASTNode *result = create_node_with_value(PRIMARY_EXPRESSION, tokens[cur++]->value);
        add_child(generic_argument_clause(), result); // try it
        return result;
    } else if ((c = literal_expression())
    || (c = self_expression())
    || (c = superclass_expression())
    || (c = closure_expression())
    || (c = parenthesized_expression())
    || (c = tuple_expression())
    || (c = wildcard_expression())
    || (c = selector_expression())) 
        return create_node_with_child(PRIMARY_EXPRESSION, c);
    return NULL;
}

// function call expressions

static ASTNode * explicit_member_expression() {
    return NULL;
}

static ASTNode * labeled_trailing_closure() {
    return NULL;
}

static ASTNode * labeled_trailing_closures() {
    return NULL;
}

static ASTNode * trailing_closure() {
    return NULL;
}

// missing syntax check
static ASTNode * function_call_argument() {
    ASTNode *c;
    if ((c = expression())) 
        return create_node_with_child(FUNCTION_CALL_ARGUMENT, c);
    if (cur >= tc + 2) return NULL;
    if (push() && tokens[cur++]->type == IDENTIFIER
    && tokens[cur++]->type == COLON && (c = expression()) && pop()) 
        return create_node_with_child(FUNCTION_CALL_ARGUMENT, c);
    pop_set();
    return NULL;
}

static ASTNode * function_call_argument_list() {
    ASTNode *c;
    if ((c = function_call_argument())) {
        ASTNode *result = create_node_with_child(FUNCTION_CALL_ARGUMENT_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(function_call_argument(), result))
                error("expected function call argument following comma");
        return result;
    }
    return NULL;
}
// here
static ASTNode * function_call_argument_clause() {
    if (push() && tokens[cur++]->type == OPEN_PAREN) {
        if (tokens[cur]->type == CLOSE_PAREN && cur++ && pop())
            return create_node(FUNCTION_CALL_ARGUMENT_CLAUSE);
        ASTNode *c;
        if ((c = function_call_argument_list())) {
            if (pop() && tokens[cur++]->type != CLOSE_PAREN)
                error("expected ) following argument list");
            return create_node_with_child(FUNCTION_CALL_ARGUMENT_CLAUSE, c);
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * function_call_expression() {
    return NULL;
}

static ASTNode * initializer_expression() {
    return NULL;
}


// subscript/post/pre expression

static ASTNode * subscript_expression() {
    return NULL;
}

static ASTNode * forced_value_expression() {
    return NULL;
}

static ASTNode * optional_chaining_expression() {
    return NULL;
}

static ASTNode * postfix_expression() {
    printf("postfix_expression %s\n", tokens[cur]->value);
    ASTNode *c;
    if ((c = primary_expression())) {
        ASTNode *result = create_node_with_child(POSTFIX_EXPRESSION, c);
        add_child(postfix_operator(), result); // optional
        return result;
    } else if ((c = function_call_expression())
    || (c = initializer_expression())
    || (c = explicit_member_expression())
    || (c = subscript_expression())
    || (c = forced_value_expression())
    || (c = optional_chaining_expression())) 
        return create_node_with_child(POSTFIX_EXPRESSION, c);
    return NULL;
}

// missing syntax check
static ASTNode * prefix_expression() {
    printf("prefix_expression %s\n", tokens[cur]->value);
    ASTNode *c;
    if ((c = prefix_operator())) {
        ASTNode *result = create_node_with_child(PREFIX_EXPRESSION, c);
        add_child(postfix_expression(), result);
        return result;
    } else if ((c = postfix_expression())) 
        return create_node_with_child(PREFIX_EXPRESSION, c);
    return NULL;
}

// misc operators

static ASTNode * type_casting_operator() {
    return NULL;
}

static ASTNode * conditional_operator() {
    return NULL;
}

static ASTNode * assignment_operator() {
    return NULL;
}

// expression

static ASTNode * binary_expression() {
    puts("binary_expression");
    ASTNode *c;
    if ((c = type_casting_operator()))
        return create_node_with_child(BINARY_EXPRESSION, c);
    if ((c = binary_operator())) {
        ASTNode *result = create_node_with_child(BINARY_EXPRESSION, c);
        if (add_child(prefix_expression(), result))
            return result;
        free(result);
    } else if ((c = assignment_operator()) || (c = conditional_operator())) {
        ASTNode *result = create_node_with_child(BINARY_EXPRESSION, c);
        add_child(try_operator(), result);
        return result;
    }
    return NULL;
}

static ASTNode * binary_expressions() {
    puts("binary_expressions");
    ASTNode *c;
    if ((c = binary_expression())) {
        puts("got binary expression inside binary_expressions");
        ASTNode *result = create_node_with_child(BINARY_EXPRESSIONS, c);
        while(add_child(binary_expression(), result));
        return result;
    }
    puts("failed to get binary_expressions");
    return NULL;
}
// missing syntax check
ASTNode * expression() {
    printf("expression %s\n", tokens[cur]->value);
    ASTNode *c;
    if ((c = try_operator())) {
        ASTNode *result = create_node_with_child(EXPRESSION, c);
        if (add_child(prefix_expression(), result)) {
            add_child(binary_expressions(), result);
            return result;
        }
    } else if ((c = prefix_expression())) {
        ASTNode *result = create_node_with_child(EXPRESSION, c);
        add_child(binary_expressions(), result);
        return result;
    }
    return NULL;
}

static ASTNode * expression_list() {
    ASTNode *c;
    if ((c = expression())) {
        ASTNode *result = create_node_with_child(EXPRESSION_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(expression(), result))
                error("expected expression following comma");
        return result;
    }
    return NULL;
}

static ASTNode * case_condition() {
    if (push() && tokens[cur++]->type == CASE) {
        ASTNode *c;
        if ((c = pattern()) && pop())
            return create_node_with_child(CASE_CONDITION, c);
    }
    pop_set();
    return NULL;
}

static ASTNode * initializer() {
    printf("initializer %s\n", tokens[cur]->value);
    if (tokens[cur]->subtype == '=' && cur++) {
        ASTNode *result = create_node(INITIALIZER);
        if (!add_child(expression(), result))
            error("expected expression following =");
        return result;
    }
    return NULL;
}

static ASTNode * pattern_initializer() {
    puts("pattern initializer");
    ASTNode *c;
    if ((c = pattern())) {
        ASTNode *result = create_node_with_child(PATTERN_INITIALIZER, c);
        add_child(initializer(), result);
        return result;
    }
    return NULL;
}

static ASTNode * pattern_initializer_list() {
    puts("pattern initializer list");
    ASTNode *c;
    if ((c = pattern_initializer())) {
        ASTNode *result = create_node_with_child(PATTERN_INITIALIZER_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(pattern_initializer(), result))
                error("expected pattern initializer following comma");
        return result;
    }
    return NULL;
}

static ASTNode * optional_binding_condition() {
    if (push() && tokens[cur]->type == LET || tokens[cur]->type == VAR) {
        ASTNode *c;
        char *value = tokens[cur++]->value;
        if ((c = pattern_initializer_list()) && pop()) {
            ASTNode *result = create_node_with_child(OPTIONAL_BINDING_CONDITION, c);
            result->value = value;
            return result;
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * condition() {
    ASTNode *c;
    if ((c = expression())
    || (c = case_condition())
    || (c = optional_binding_condition()))
        return create_node_with_child(CONDITION, c);
    return NULL;
}

static ASTNode * condition_list() {
    ASTNode *c;
    if ((c = condition())) {
        ASTNode *result = create_node_with_child(CONDITION_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(condition(), result))
                error("expected condition following comma");
        return result;
    }
    return NULL;
}

// statements

static ASTNode * code_block() {
    if (tokens[cur]->type == OPEN_CURL_BRACE && cur++) {
        if (tokens[cur]->type == CLOSE_CURL_BRACE && cur++)
            return create_node(CODE_BLOCK);
        ASTNode *result = create_node(CODE_BLOCK);
        if (!add_child(statements(), result))
            error("expected statements inside code block");
        if (tokens[cur++]->type != CLOSE_CURL_BRACE)
            error("expected } following code block");
        return result;
    }
    return NULL;
}

// loops

static ASTNode * for_in_statement() {
    return NULL;
}

static ASTNode * while_statement() {
    if (tokens[cur]->type == WHILE && cur++) {
        ASTNode *result = create_node(WHILE_STATEMENT);
        if (!add_child(condition_list(), result))
            error("expected condition for while");
        if (!add_child(code_block(), result))
            error("expected code block following while");
        return result;
    }
    return NULL;
}

static ASTNode * repeat_while_statement() {
    if (tokens[cur]->type == REPEAT && cur++) {
        ASTNode *result = create_node(REPEAT_WHILE_STATEMENT);
        if (!add_child(code_block(), result))
            error("expected code block following repeat");
        if (tokens[cur++]->type != WHILE)
            error("expected while expression with repeat");
        if (!add_child(expression(), result))
            error("expected expression in while");
        return result;
    }
    return NULL;
}

static ASTNode * loop_statement() {
    ASTNode *c;
    if ((c = for_in_statement())
    || (c = while_statement())
    || (c = repeat_while_statement()))
        return create_node_with_child(LOOP_STATEMENT, c);
    return NULL;
}


static ASTNode * else_clause() {
    if (tokens[cur]->type == ELSE && cur++) {
        ASTNode *result = create_node(ELSE_CLAUSE);
        if (!add_child(code_block(), result) && !add_child(if_statement(), result))
            error("expected code block or if statement following else");
        return result;
    }
    return NULL;
}

ASTNode * if_statement() {
    if (tokens[cur]->type == IF && cur++) {
        ASTNode *result = create_node(IF_STATEMENT);
        if (!add_child(condition_list(), result))
            error("expected condition for if statement");
        if (!add_child(code_block(), result))
            error("expected code block following if statement");
        // optional
        add_child(else_clause(), result);
        return result;
    }
    return NULL;
}

static ASTNode * guard_statement() {
    if (tokens[cur]->type == GUARD && cur++) {
        ASTNode *result = create_node(GUARD_STATEMENT);
        if (!add_child(condition_list(), result))
            error("expected condition list following guard");
        if (tokens[cur++]->type != ELSE)
            error("expected else following guard condition list");
        if (!add_child(code_block(), result))
            error("expected code block for guard statement");
        return result;
    }
    return NULL;
}

// switch

static ASTNode * switch_cases() {
    return NULL;
}

static ASTNode * switch_case() {
    return NULL;
}

static ASTNode * case_label() {
    return NULL;
}

static ASTNode * case_item() {
    return NULL;
}

static ASTNode * case_item_list() {
    ASTNode *c;
    if ((c = case_item())) {
        ASTNode *result = create_node_with_child(CASE_ITEM_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(case_item(), result))
                error("expected case item following comma");
        return result;
    }
    return NULL;
}

static ASTNode * default_label() {
    return NULL;
}

static ASTNode * switch_statement() {
    return NULL;
}

static ASTNode * branch_statement() {
    ASTNode *c;
    if ((c = if_statement())
    || (c = guard_statement())
    || (c = switch_statement()))
        return create_node_with_child(BRANCH_STATEMENT, c);
    return NULL;
}

// where

static ASTNode * where_clause() {
    return NULL;
}

static ASTNode * where_expression() {
    return NULL;
}

static ASTNode * label_name() {
    if (tokens[cur]->type == IDENTIFIER && cur++)
        return create_node(LABEL_NAME);
    return NULL;
}

static ASTNode * statement_label() {
    ASTNode *c;
    if (push() && (c = label_name()) && tokens[cur++]->type == COLON && pop()) 
        return create_node_with_child(STATEMENT_LABEL, c);
    pop_set();
    return NULL;
}

// do/catch

static ASTNode * do_statement() {
    return NULL;
}

static ASTNode * catch_clause() {
    return NULL;
}

static ASTNode * catch_clauses() {
    return NULL;
}

static ASTNode * catch_pattern() {
    return NULL;
}

static ASTNode * catch_pattern_list() {
    ASTNode *c;
    if ((c = catch_pattern())) {
        ASTNode *result = create_node_with_child(CATCH_PATTERN_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(catch_pattern(), result))
                error("expected catch pattern following comma");
        return result;
    }
    return NULL;
}

static ASTNode * labeled_statement() {
    ASTNode *c;
    if (push() && (c = statement_label())) {
        ASTNode *result = create_node_with_child(LABELED_STATEMENT, c);
        if((add_child(loop_statement(), result)
        || add_child(if_statement(), result)
        || add_child(switch_statement(), result)
        || add_child(do_statement(), result)) && pop())
            return result;
    }
    pop_set();
    return NULL;
}

// control transfer

static ASTNode * break_statement() {
    if (tokens[cur]->type == BREAK && cur++) {
        ASTNode *result = create_node(BREAK_STATEMENT);
        add_child(label_name(), result);
        return result;
    }
    return NULL;
}

static ASTNode * continue_statement() {
    if (tokens[cur]->type == CONTINUE && cur++) {
        ASTNode *result = create_node(CONTINUE_STATEMENT);
        add_child(label_name(), result);
        return result;
    }
    return NULL;
}

static ASTNode * fallthrough_statement() {
    if (tokens[cur]->type == FALLTHROUGH && cur++)
        return create_node(FALLTHROUGH_STATEMENT);
    return NULL;
}

static ASTNode * return_statement() {
    if (tokens[cur]->type == RETURN && cur++) {
        ASTNode *result = create_node(RETURN_STATEMENT);
        if (!add_child(expression(), result))
            error("expected expression following return");
        return result;
    }
    return NULL;
}

static ASTNode * throw_statement() {
    if (tokens[cur]->type == THROW && cur++) {
        ASTNode *result = create_node(THROW_STATEMENT);
        if (!add_child(expression(), result))
            error("expected expression following throw");
        return result;
    }
    return NULL;
}

static ASTNode * control_transfer_statement() {
    ASTNode *c;
    if ((c = break_statement())
    || (c = continue_statement())
    || (c = fallthrough_statement())
    || (c = return_statement())
    || (c = throw_statement()))
        return create_node_with_child(CONTROL_TRANSFER_STATEMENT, c);
    return NULL;
}

static ASTNode * statement() {
    printf("statement called with token %s\n", tokens[cur]->value);
    ASTNode *c;
    if ((c = expression())
    || (c = declaration())
    || (c = loop_statement())
    || (c = branch_statement())
    || (c = labeled_statement())
    || (c = control_transfer_statement())
    || (c = do_statement())) 
        return create_node_with_child(STATEMENT, c);
    return NULL;
}

ASTNode * statements() {
    puts("statements");
    ASTNode *c;
    if ((c = statement())) {
        ASTNode *result = create_node_with_child(STATEMENTS, c);
        while(add_child(statement(), result));
        return result;
    }
    return NULL;
}

// declarations

// i think the only context this can be called, it will be
// invalid if there is a let without pattern initializer list
static ASTNode * constant_declaration() {
    printf("constant declaration %s\n", tokens[cur]->value);
    if (tokens[cur]->type == LET && cur++) {
        ASTNode *result = create_node(CONSTANT_DECLARATION);
        if (!add_child(pattern_initializer_list(), result))
            error("expected pattern initializer list following let");
        return result;
    } 
    ASTNode *c;
    if ((c = declaration_modifiers())) {
        if (tokens[cur]->type == LET && cur++) {
            ASTNode *result = create_node_with_child(CONSTANT_DECLARATION, c);
            if (!add_child(pattern_initializer_list(), result))
                error("expected pattern initializer list following let");
            return result;
        } 
    }
    return NULL;
}

static ASTNode * wildcard_pattern() {
    if (tokens[cur]->type == WILDCARD && cur++)
        return create_node(WILCARD_PATTERN);
    return NULL;
}

static ASTNode * identifier_pattern() {
    if (tokens[cur]->type == IDENTIFIER)
        return create_node_with_value(IDENTIFIER_PATTERN, tokens[cur++]->value);
    return NULL;
}

static ASTNode * tuple_pattern_element() {
    return NULL;
}

static ASTNode * tuple_pattern_element_list() {
    return NULL;
}

static ASTNode * tuple_pattern() {
    return NULL;
}

static ASTNode * value_binding_pattern() {
    ASTNode *c;
    if (push() && tokens[cur++]->type == IDENTIFIER && (c = pattern()) && pop()) 
        return create_node_with_child(VALUE_BINDING_PATTERN, c);
    pop_set();
    return NULL;
}

static ASTNode * enum_case_pattern() {
    return NULL;
}
// have to check this before identifier pattern
static ASTNode * optional_pattern() {
    ASTNode *c;
    if (push() && (c = identifier_pattern()) && tokens[cur++]->subtype == '?' && pop())
        return create_node_with_child(OPTIONAL_PATTERN, c);
    return NULL;
}

static ASTNode * type_casting_pattern() {
    return NULL;
}

static ASTNode * expression_pattern() {
    ASTNode *c;
    if ((c = expression()))
        return create_node_with_child(EXPRESION_PATTERN, c);
    return NULL;
}

ASTNode * pattern() {
    puts("pattern");
    ASTNode *c;
    if ((c = wildcard_pattern())
    || (c = identifier_pattern())
    || (c = tuple_pattern())) {
        ASTNode *result = create_node_with_child(PATTERN, c);
        add_child(type_annotation(), result); // optional
        return result;
    } else if ((c = value_binding_pattern())
    || (c = enum_case_pattern())
    || (c = optional_pattern())
    || (c = type_casting_pattern())
    || (c = expression_pattern())) 
        return create_node_with_child(PATTERN, c);
    return NULL;
}

static ASTNode * typedef_declaration() {
    return NULL;
}

// variable dec

static ASTNode * variable_name() {
    if (tokens[cur]->type == IDENTIFIER)
        return create_node_with_value(VARIABLE_NAME, tokens[cur++]->value);
    return NULL;
}

static ASTNode * variable_declaration_head() {
    printf("variable dec head called with token: %s\n", tokens[cur]->value);
    if (tokens[cur]->type == VAR && cur++) 
        return create_node(VARIABLE_DECLARATION_HEAD);
    ASTNode *c;
    if (push() && (c = declaration_modifiers()))
        if (tokens[cur]->type == VAR && cur++ && pop())
            return create_node_with_child(VARIABLE_DECLARATION_HEAD, c);
    pop_set();
    return NULL;
}

static ASTNode * getter_clause() {
    if (tokens[cur]->type == GET && cur++) {
        ASTNode *result = create_node(GETTER_CLAUSE);
        if (!add_child(code_block(), result))
            error("expected code block folling get");
        return result;
    }
    return NULL;
}

static ASTNode * setter_clause() {
    if (tokens[cur]->type == SET && cur++) {
        ASTNode *result = create_node(SETTER_CLAUSE);
        if (tokens[cur]->type == IDENTIFIER)
            result->value = tokens[cur++]->value;
        if (!add_child(code_block(), result))
            error("expected code block folling set");
        return result;
    }
    return NULL;
}

static ASTNode * getter_setter_block() {
    ASTNode *c;
    if ((c = code_block()))
        return create_node_with_child(GETTER_SETTER_BLOCK, c);
    if (push() && tokens[cur++]->type == OPEN_CURL_BRACE) {
        if ((c = getter_clause())) {
            ASTNode *result = create_node_with_child(GETTER_SETTER_BLOCK, c);
            add_child(setter_clause(), result); // optional
            if (tokens[cur++]->type != CLOSE_CURL_BRACE)
                error("expected } to terminate getter_setter_block");
            return result;
        } else if ((c = setter_clause())) {
            ASTNode *result = create_node_with_child(GETTER_SETTER_BLOCK, c);
            if (!add_child(getter_clause(), result))
                error("variable cannot be set only");
            if (tokens[cur++]->type != CLOSE_CURL_BRACE)
                error("expected } to terminate getter_setter_block");
            return result;
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * getter_setter_keyword_block() {
    if (push() && tokens[cur++]->type == OPEN_CURL_BRACE) {
        if (tokens[cur]->type == GET && cur++) {
            ASTNode *result = create_node(GETTER_SETTER_KEYWORD_BLOCK);
            if (tokens[cur]->type == SET && cur++)
                result->value = "getset";
            else
                result->value = "get";
            return result;
        } else if (tokens[cur]->type == SET && cur++) {
            ASTNode *result = create_node_with_value(GETTER_SETTER_KEYWORD_BLOCK, "set");
            if (tokens[cur++]->type != GET)
                error("variable cannot be set only");
            return result;
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * willset_didset_block() {
    return NULL;
}

static ASTNode * willset_clause() {
    return NULL;
}

static ASTNode * didset_clause() {
    return NULL;
}

static ASTNode * variable_declaration() {
    printf("variable_declaration %s\n", tokens[cur]->value);
    ASTNode *c;
    if ((c = variable_declaration_head())) {
        puts("got dec head"); // this isn't happening when it should
        ASTNode *result = create_node_with_child(VARIABLE_DECLARATION, c);
        if (add_child(pattern_initializer_list(), result))
            return result;
        else if (add_child(variable_name(), result)) {
            if (add_child(initializer(), result)) {
                if (!add_child(willset_didset_block(), result))
                    error("expected willset following initializer");
                return result;
            } else if (add_child(type_annotation(), result)) {
                if (add_child(code_block(), result)
                || add_child(getter_setter_block(), result)
                || add_child(getter_setter_keyword_block(), result))
                    return result;
                add_child(initializer(), result);
                if (!add_child(willset_didset_block(), result))
                    error("expected willset following initializer");
                return result;
            }
        } else
            error("expected variable name");
    }
    return NULL;
}

// function dec

static ASTNode * function_head() {
    if (tokens[cur]->type == FUNC && cur++)
        return create_node(FUNCTION_HEAD);
    ASTNode *c;
    if (push() && (c = declaration_modifiers()) && tokens[cur++]->type == FUNC && pop())
        return create_node_with_child(FUNCTION_HEAD, c);
    pop_set();
    return NULL;
}

static ASTNode * function_name() {
    if (tokens[cur]->type == IDENTIFIER || tokens[cur]->type == OPERATOR)
        return create_node_with_value(FUNCTION_NAME, tokens[cur++]->value);
    return NULL;
}

static ASTNode * function_signature() {
    return NULL;
}


static ASTNode * parameter() {
    return NULL;
}

static ASTNode * parameter_list() {
    ASTNode *c;
    if ((c = parameter())) {
        ASTNode *result = create_node_with_child(PARAMETER_LIST, c);
        while(cur < tc && tokens[cur]->type == COMMA) 
            if (++cur && !add_child(parameter(), result))
                error("expected parameter following comma");
        return result;
    }
    return NULL;
}

static ASTNode * parameter_clause() {
    return NULL;
}

static ASTNode * function_declaration() {
    return NULL;
}

static ASTNode * enum_declaration() {
    return NULL;
}

static ASTNode * struct_declaration() {
    return NULL;
}

static ASTNode * class_declaration() {
    return NULL;
}

static ASTNode * protocol_declaration() {
    return NULL;
}

static ASTNode * initializer_declaration() {
    return NULL;
}

static ASTNode * deinitializer_declaration() {
    return NULL;
}

static ASTNode * extension_declaration() {
    return NULL;
}

static ASTNode * subscript_declaration() {
    return NULL;
}

static ASTNode * operator_declaration() {
    return NULL;
}

static ASTNode * precedence_group_declaration() {
    return NULL;
}

ASTNode * declaration() {
    printf("declaration: %s\n", tokens[cur]->value);
    ASTNode *c;
    if ((c = constant_declaration())
    || (c = variable_declaration())
    || (c = typedef_declaration())
    || (c = function_declaration())
    || (c = enum_declaration())
    || (c = struct_declaration())
    || (c = class_declaration())
    || (c = protocol_declaration())
    || (c = initializer_declaration())
    || (c = deinitializer_declaration())
    || (c = extension_declaration())
    || (c = subscript_declaration())
    || (c = operator_declaration())
    || (c = precedence_group_declaration())) 
        return create_node_with_child(DECLARATION, c);
    return NULL;
}

static ASTNode * declarations() {
    ASTNode *c;
    if ((c = declaration())) {
        ASTNode *result = create_node_with_child(DECLARATIONS, c);
        while(add_child(declaration(), result));
        return result;
    }
    return NULL;
}

static ASTNode * top_level_declaration() {
    puts("inside tld");
    ASTNode *c;
    if ((c = statements())) 
        return create_node_with_child(TOP_LEVEL_DECLARATION, c);
    return NULL;
}

ASTNode * parse(Token **tks, size_t token_count) {
    tokens = tks;
    tc = token_count;

    // populate primitive table
    primitive_types = map_init_size(PRIMITIVE_TYPES);
    type_names = map_init_size(256);

    for (int i = 0; i < PRIMITIVE_TYPES; i++) {
        map_insert(primitive_types_array[i], primitive_types_array[i], primitive_types);
    }

    // entry point 
    return top_level_declaration();
}