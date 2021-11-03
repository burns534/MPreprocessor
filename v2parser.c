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
    if (cursor_stack_top > 0 && cursor_stack_top--)
        cur = cursor_stack_top;
    return 1;
}

static inline int push() {
    if (cursor_stack_top + 1 < CURSOR_STACK_DEPTH)
        cursor_stack[cursor_stack_top++] = cur;
    else
        perror("stack overflow");
    return 1;
}

static bool add_child(ASTNode *c, ASTNode *p) {
    if (!c || !p) return false;
    if (p->child_count > 0.6 * p->children_size)
        p->children = realloc(p->children, p->children_size *= 2);
    p->children[p->child_count++] = c;
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

// static inline void set_children(ASTNode *n) {
//     n->children = malloc(bi * sizeof(ASTNode *));
//     n->child_count = bi;
//     bi = 0;
//     for (int i = 0; i < bi; i++)
//         n->children[i] = buf[i];
// }

// static inline void set_value(char *s, ASTNode *n) {
//     size_t len = strlen(s);
//     n->value = malloc(len + 1);
//     strcpy(n->value, s);
//     n->value[len] = 0;
// }

static void print_node(ASTNode *node) {
    if (!node) return;
    printf("%s: %s", node_type_to_string(node->type), node->value);
}

static inline void print_tabs(int count) {
    for (int i = 0; i < count; i++)
        putchar('\t');
}

static void print_tree_util(ASTNode *node, int level) {
    if (node) {
        print_tabs(level);
        printf("Node: %s, value: %s\n", node_type_to_string(node->type), node->value);

        if (node->child_count) {
            print_tabs(level);
            printf("Children:\n");
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
    if (push() && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, "-") == 0) {
        // cases 1 and 3
        if (tokens[cur]->type == INTEGER_LITERAL || tokens[cur]->type == FLOATING_POINT_LITERAL) {
            ASTNode *result = create_node(NUMERIC_LITERAL);
            char *buffer = malloc(strlen(tokens[cur]->value) + 2);
            buffer[0] = '-';
            strcat(buffer + 1, tokens[cur]->value);
            result->value = buffer;
            pop();
            return result;
        }
    } else {
        pop_set();
    }

    if (tokens[cur]->type == INTEGER_LITERAL || tokens[cur]->type == FLOATING_POINT_LITERAL) {
        ASTNode *result = create_node(NUMERIC_LITERAL);
        result->value = tokens[cur]->value;
        return result;
    }
    return NULL;
}

static inline ASTNode * boolean_literal() {
    if (tokens[cur]->type == TRUE || tokens[cur]->type == FALSE) {
        ASTNode *result = create_node(BOOLEAN_LITERAL);
        result->value = tokens[cur++]->value;
        return result;
    }
    return NULL;
}

static inline ASTNode * nil_literal() {
    if (tokens[cur]->type == NIL)
        return create_node_with_value(NIL_LITERAL, tokens[cur++]->value);
    return NULL;
}

static inline ASTNode * string_literal() {
    if (tokens[cur]->type == STATIC_STRING_LITERAL || tokens[cur]->type == INTERPOLATED_STRING_LITERAL) {
        ASTNode *result = create_node(STRING_LITERAL);
        result->value = tokens[cur++]->value;
        return result;
    }
    return NULL;
}

static ASTNode * literal() {
    ASTNode *c;
    if ((c = numeric_literal())
    || (c = string_literal())
    || (c = boolean_literal())
    || (c = nil_literal())) 
        return create_node_with_child(LITERAL, c);
    return NULL;
}

static ASTNode * access_level_modifier() {
    if (tokens[cur]->type == PRIVATE || tokens[cur]->type == PUBLIC)
        return create_node_with_value(ACCESS_LEVEL_MODIFIER, tokens[cur++]->value);
    return NULL;
}

static ASTNode * binary_operator() {
    if (tokens[cur]->type == OPERATOR) 
        return create_node_with_value(BINARY_OPERATOR, tokens[cur++]->value);
    return NULL;
}

static ASTNode * prefix_operator() {
    if (tokens[cur]->type == OPERATOR) 
        return create_node_with_value(PREFIX_OPERATOR, tokens[cur++]->value);
    return NULL;
}

static ASTNode * postfix_operator() {
    if (tokens[cur]->type == OPERATOR) 
        return create_node_with_value(POSTFIX_OPERATOR, tokens[cur++]->value);
    return NULL;
}

static ASTNode * type_name() {
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
    if (push() 
    && tokens[cur++]->subtype == '<'
    && (c = generic_argument_list())
    && tokens[cur++]->subtype == '>' && pop()) {
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
    if (push() && tokens[cur++]->type == OPEN_PAREN) {
        if (tokens[cur]->type == CLOSE_PAREN && ++cur)
            return create_node(TUPLE_TYPE);
        ASTNode *c;
        if ((c = tuple_type_element())) {
            if (tokens[cur++]->type != COMMA)
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
    if (tokens[cur]->type == OPEN_PAREN) {
        if (tokens[++cur]->type == CLOSE_PAREN && ++cur)
            return create_node(FUNCTION_TYPE_ARGUMENT_CLAUSE);
        ASTNode *c;
        if ((c = function_type_arugment_list())) {
            ASTNode *result = create_node_with_child(FUNCTION_TYPE_ARGUMENT_CLAUSE, c);
            // variadic
            if (tokens[cur]->subtype == '*')
                result->value = tokens[cur++]->value;
            
            if (tokens[cur++]->type != CLOSE_PAREN)
                error("expected )");

            return result;
        }
    }
    return NULL;
}

static ASTNode * function_type() {
    ASTNode *c;
    if ((c == function_type_argument_clause())) {
        ASTNode *result = create_node_with_child(FUNCTION_TYPE, c);
        if (tokens[cur]->type == THROWS) {
            result->value = "throws";
            cur++;
        }

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
    if (push() && tokens[cur++]->type == OPEN_SQ_BRACE && (c = type()) && tokens[cur++]->type == CLOSE_SQ_BRACE && pop())
        return create_node_with_child(ARRAY_TYPE, c);
    pop_set();
    return NULL;
}

static ASTNode * dictionary_type() {
    ASTNode *c;
    if (push() && tokens[cur++]->type == OPEN_SQ_BRACE && (c = type()) && tokens[cur++]->type == COLON && pop()) {
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

static ASTNode * protocol_composition_continuation() {
    return NULL;
}

static ASTNode * protocol_composition_type() {
    return NULL;
}

static ASTNode * any_type() {
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

static ASTNode * type_inheritance_clause() {
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
    if (push() && tokens[cur++]->type == OPEN_PAREN
    && (c = type()) && tokens[cur++]->type == CLOSE_PAREN && pop()) {
        ASTNode *result = create_node_with_child(TYPE, c);
        result->value = "parenthesized";
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * declaration_modifier() {
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
    if (tokens[cur]->type == TRY) {
        ASTNode *result = create_node(TRY_OPERATOR);
        cur++;
        if (tokens[cur]->type == OPERATOR && (strcmp(tokens[cur]->value, "?") == 0 || strcmp(tokens[cur]->value, "!") == 0)) 
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

// here
static ASTNode * array_literal() {
    if (push() && tokens[cur]->type == OPEN_SQ_BRACE) {
        cur++;
        if ((c = array_literal_items())) {
            bi++;
            ASTNode *result = create_node(ARRAY_LITERAL);
            if (tokens[cur++]->type != CLOSE_SQ_BRACE && pop())
                error("expected ] following array literal litems");
            set_children(result);
            return result;
        }

        if (tokens[cur++]->type == CLOSE_SQ_BRACE && pop()) 
            return create_node(ARRAY_LITERAL);
    }
    pop_set();
    return NULL;
}

static ASTNode * dictionary_literal_item() {
    if (push() && (c = expression())) {
        if (tokens[cur++]->type == COLON && (buf[1] = expression()) && pop()) {
            ASTNode *result = create_node(DICTIONARY_LITERAL_ITEM);
            bi = 2;
            set_children(result);
            return result;
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * dictionary_literal_items() {
    if ((c = dictionary_literal_item())) {
        bi++;
        ASTNode *result = create_node(DICTIONARY_LITERAL_ITEMS);

        while(cur < tc && tokens[cur]->type == COMMA) {
            cur++;
            if (!(buf[bi++] = dictionary_literal_item()))
                error("expected dictionary literal item following comma");
        }

        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * dictionary_literal() {
    if (push() && tokens[cur]->type == OPEN_SQ_BRACE) {
        cur++;
        if ((c = dictionary_literal_items())) {
            bi++;
            ASTNode *result = create_node(ARRAY_LITERAL);
            if (tokens[cur++]->type != CLOSE_SQ_BRACE && pop())
                error("expected ] following dictionary literal litems");
            set_children(result);
            return result;
        }

        if (tokens[cur++]->type == COLON) {
            if (tokens[cur++]->type != CLOSE_SQ_BRACE && pop())
                error("expected ] following colon in dictionary literal");
            return create_node(ARRAY_LITERAL);
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * literal_expression() {
    if ((c = literal())
    || (c = array_literal()) 
    || (c = dictionary_literal())) {
        bi = 1;
        ASTNode *result = create_node(LITERAL_EXPRESSION);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * self_initializer_expression() {
    if (push() && tokens[cur++]->type == SELF 
    && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, ".") == 0
    && tokens[cur++]->type == INIT && pop())
        return create_node(SELF_INITIALIZER_EXPRESSION);
    pop_set();
    return NULL;
}

static ASTNode * self_subscript_expression() {
    return NULL;
}

static ASTNode * self_method_expression() {
    return NULL;
}

static ASTNode * self_expression() {
    if ((c = self_method_expression())
    || (c = self_subscript_expression())
    || (c = self_initializer_expression())) {
        bi = 1;
        ASTNode *result = create_node(SELF_EXPRESSION);
        set_children(result);
        return result;
    }
    if (tokens[cur]->type == SELF)
        return create_node_with_value(SELF_EXPRESSION, tokens[cur++]->value);
    return NULL;
}

static ASTNode * superclass_initializer_expression() {
    if (push() && tokens[cur++]->type == SUPER 
    && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, ".") == 0 
    && tokens[cur++]->type == INIT && pop())
        return create_node(SUPERCLASS_INITIALIZER_EXPRESSION);
    pop_set();
    return NULL;
}

static ASTNode * superclass_subscript_expression() {
    return NULL;
}

static ASTNode * superclass_method_expression() {
    return NULL;
}

static ASTNode * superclass_expression() {
    if ((c = superclass_method_expression())
    || (c = superclass_subscript_expression())
    || (c = superclass_initializer_expression())) {
        bi = 1;
        ASTNode *result = create_node(SUPERCLASS_EXPRESSION);
        set_children(result);
        return result;
    }
    if (tokens[cur]->type == SUPER)
        return create_node_with_value(SUPERCLASS_EXPRESSION, tokens[cur++]->value);
    return NULL;
}

// closures

static ASTNode * closure_parameter_name() {
    if (tokens[cur]->type == IDENTIFIER) 
        return create_node_with_value(CLOSURE_PARAMETER_NAME, tokens[cur++]->value);
    return NULL;
}
// should really have subtypes for operator token or something better
static ASTNode * closure_parameter() {
    if ((c = closure_parameter_name())) {
        ASTNode *result = create_node(CLOSURE_PARAMETER);
        bi++;
        if ((buf[bi] = type_annotation())) bi++;
        set_children(result);
        if (push() && cur + 2 < tc
        && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, ".") == 0
        && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, ".") == 0
        && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, ".") == 0 && pop()) {
            // variadic
            result->value = "...";
            return result;
        }
        pop_set();
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

static ASTNode * identifier_list() {
    if (tokens[cur]->type == IDENTIFIER) {
        while(cur < tc && tokens[cur]->type == COMMA) {
            cur++;
            if (tokens[cur++]->type != IDENTIFIER)
                error("expected identifier following comma");
        }
        ASTNode *result = create_node(IDENTIFIER_LIST);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * closure_parameter_clause() {
    if ((c = identifier_list())) {
        ASTNode *result = create_node(CLOSURE_PARAMETER_CLAUSE);
        bi++;
        set_children(result);
        return result;
    } else if (push() && tokens[cur++]->type == OPEN_PAREN) {
        if (tokens[cur++]->type == CLOSE_PAREN && pop()) 
            return create_node(CLOSURE_PARAMETER_CLAUSE);
        if ((c = closure_parameter_list())) {
            bi++;
            if (tokens[cur++]->type != CLOSE_PAREN && pop())
                error("expected ) following closure parameter list");
            ASTNode *result = create_node(CLOSURE_PARAMETER_CLAUSE);
            set_children(result);
            return result;
        }
    }
    pop_set();
    return NULL;
}

static ASTNode * function_result() {
    if ((c = type())) {
        bi++;
        ASTNode *result = create_node(FUNCTION_RESULT);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * closure_signature() {
    if (tokens[cur]->type == IN) {
        cur++;
        return create_node(CLOSURE_SIGNATURE); 
    } else if ((c = closure_parameter_clause())) {
        bi++;
        if (tokens[cur]->type == IN) {
            ASTNode *result = create_node_with_value(CLOSURE_SIGNATURE, tokens[cur++]->value);
            set_children(result);
            return result;
        } else if ((buf[bi] = function_result())) {
            if (tokens[cur++]->type != IN)
                error("expected \"in\" keyword following type");
            ASTNode *result = create_node(CLOSURE_SIGNATURE);
            bi++;
            set_children(result);
            return result;
        } else if (push() && tokens[cur++]->type == THROWS) {
            ASTNode *result = create_node_with_value(CLOSURE_SIGNATURE, "throws");
            if (tokens[cur]->type == IN) {
                cur++;
            } else if ((buf[bi] = function_result())) {
                bi++;
                if (tokens[cur++]->type != IN)
                    error("expected \"in\" keyword following type");
            }

            pop();
            set_children(result);
            return result;
        }
        pop_set();
    }
    return NULL;
}

static ASTNode * closure_expression() {
    if (push() && tokens[cur++]->type == OPEN_CURL_BRACE) {
        if (tokens[cur]->type == CLOSE_CURL_BRACE && pop()) {
            cur++;
            return create_node(CLOSURE_EXPRESSION);
        } else if ((c = closure_signature()) && ++bi && pop()) {
            ASTNode *result = create_node(CLOSURE_EXPRESSION);
            if ((buf[bi] = statements())) bi++;
            set_children(result);
            return result;

        } else if ((c = statements()) && ++bi && pop()) {
            ASTNode *result = create_node(CLOSURE_EXPRESSION);
            set_children(result);
            return result;
        }
    }
    pop_set();
    return NULL;
}

// other expressions

static ASTNode * parenthesized_expression() {
    if (push() && tokens[cur++]->type == OPEN_PAREN && ((c = expression()) && tokens[cur++]->type == CLOSE_PAREN) && pop()) {
        ASTNode *result = create_node(PARENTHESIZED_EXPRESSION);
        bi = 1;
        set_children(result);
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * tuple_element() {
    if ((c = expression())) {
        ASTNode *result = create_node(TUPLE_ELEMENT);
        set_children(result);
        return result;
    } else if (push() && tokens[cur]->type == IDENTIFIER) {
        ASTNode *result = create_node_with_value(TUPLE_ELEMENT, tokens[cur++]->value);
        if (tokens[cur++]->type == COLON && (c = expression()) && pop()) {
            set_children(result);
            return result;
        }
        free(result);
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
    return NULL;
}

static ASTNode * wildcard_expression() {
    if (tokens[cur]->type == WILDCARD) 
        return create_node_with_value(WILDCARD_EXPRESSION, tokens[cur++]->value);
    return NULL;
}

static ASTNode * selector_expression() {
    if (push() && tokens[cur++]->type == HASHTAG
    && tokens[cur++]->type == SELECTOR && tokens[cur++]->type == OPEN_PAREN
    && (c = expression()) && tokens[cur++]->type == CLOSE_PAREN && ++bi && pop()) {
        ASTNode *result = create_node(SELECTOR_EXPRESSION);
        set_children(result);
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * primary_expression() {
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

// this might be wrong
static ASTNode * function_call_argument() {
    if ((c = expression()) && ++bi) {
        ASTNode *result = create_node(FUNCTION_CALL_ARGUMENT);
        set_children(result);
        return result;
    } else if (push() && tokens[cur++]->type == IDENTIFIER
    && tokens[cur++]->type == COLON && (c = expression()) && ++bi && pop()) {
        ASTNode *result = create_node(FUNCTION_CALL_ARGUMENT);
        set_children(result);
        return result;
    }
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

static ASTNode * function_call_argument_clause() {
    if (push() && tokens[cur++]->type == OPEN_PAREN) {
        if (tokens[cur++]->type == CLOSE_PAREN && pop())
            return create_node(FUNCTION_CALL_ARGUMENT_CLAUSE);
        else if ((c = function_call_argument_list()) && ++bi) {
            if (tokens[cur++]->type != CLOSE_PAREN && pop())
                error("expected ) following argument list");
            ASTNode *result = create_node(FUNCTION_CALL_ARGUMENT_CLAUSE);
            set_children(result);
            return result;
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
    if ((c = primary_expression())) {
        ASTNode *result = create_node(POSTFIX_EXPRESSION);
        bi++;
        if ((buf[bi] = postfix_operator())) bi++;
        set_children(result);
        return result;
    } else if ((c = function_call_expression())
    || (c = initializer_expression())
    || (c = explicit_member_expression())
    || (c = subscript_expression())
    || (c = forced_value_expression())
    || (c = optional_chaining_expression())) {
        ASTNode *result = create_node(POSTFIX_EXPRESSION);
        bi++;
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * prefix_expression() {
    if ((c = prefix_operator())) {
        bi++;
        // maybe I should throw an error here?
        // i will for now
        if (!(buf[bi++] = postfix_expression()))
            error("expexted expression following prefix operator");
        
        ASTNode *result = create_node(PREFIX_EXPRESSION);
        set_children(result);
        return result;
    } else if ((c = postfix_expression())) {
        bi++;
        ASTNode *result = create_node(PREFIX_EXPRESSION);
        set_children(result);
        return result;
    }
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
    return NULL;
}

static ASTNode * binary_expressions() {
    if ((c = binary_expression()) && ++bi) {
        while((buf[bi] = binary_expression())) bi++;
        ASTNode *result = create_node(BINARY_EXPRESSIONS);
        set_children(result);
        return result;
    }
    return NULL;
}

ASTNode * expression() {
    if ((c = try_operator())) {
        bi++;
        if (!(buf[bi++] = prefix_expression()))
            error("expected expression following try oeprator");
        ASTNode *result = create_node(EXPRESSION);
        if ((buf[bi] = binary_expressions())) bi++;
        set_children(result);
        puts("got expression");
        return result;
    } else if ((c = prefix_expression())) {
        ASTNode *result = create_node(EXPRESSION);
        puts("got expression");
        bi++;
        if ((buf[bi] = binary_expressions())) bi++;
        set_children(result);
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

// statements

static ASTNode * loop_statement() {
    return NULL;
}

static ASTNode * for_in_statement() {
    return NULL;
}

static ASTNode * while_statement() {
    return NULL;
}

static ASTNode * condition() {
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

static ASTNode * case_condition() {
    return NULL;
}

static ASTNode * optional_binding_condition() {
    return NULL;
}

static ASTNode * repeat_while_statement() {
    return NULL;
}

static ASTNode * branch_statement() {
    return NULL;
}

static ASTNode * if_statement() {
    return NULL;
}

static ASTNode * else_clause() {
    return NULL;
}

static ASTNode * guard_statement() {
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

// where

static ASTNode * where_clause() {
    return NULL;
}

static ASTNode * where_expression() {
    return NULL;
}

static ASTNode * labeled_statement() {
    return NULL;
}

static ASTNode * statement_label() {
    return NULL;
}

static ASTNode * label_name() {
    return NULL;
}

// control transfer

static ASTNode * control_transfer_statement() {
    return NULL;
}

static ASTNode * break_statement() {
    return NULL;
}

static ASTNode * continue_statement() {
    return NULL;
}

static ASTNode * fallthrough_statement() {
    return NULL;
}

static ASTNode * return_statement() {
    return NULL;
}

static ASTNode * throw_statement() {
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

static ASTNode * statement() {
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
    ASTNode *c;
    if ((c = statement())) {
        puts("getting statements");
        ASTNode *result = create_node_with_child(STATEMENTS, c);
        while(add_child(statement(), result));
        return result;
    }
    return NULL;
}

// declarations

static ASTNode * code_block() {
    return NULL;
}

static ASTNode * constant_declaration() {
    if (tokens[cur])
    return NULL;
}

static ASTNode * pattern() {
    return NULL;
}

static ASTNode * initializer() {
    ASTNode *c;
    if ((c = expression())) 
        return create_node_with_child(INITIALIZER, c);
    return NULL;
}

static ASTNode * pattern_initializer() {
    ASTNode *c;
    if ((c = pattern())) {
        ASTNode *result = create_node_with_child(PATTERN_INITIALIZER, c);
        add_child(initializer(), result);
        return result;
    }
    return NULL;
}

static ASTNode * pattern_initializer_list() {
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

static ASTNode * typedef_declaration() {
    return NULL;
}

// variable dec

static ASTNode * variable_declaration() {
    return NULL;
}

static ASTNode * variable_declaration_head() {
    return NULL;
}

static ASTNode * getter_setter_block() {
    return NULL;
}

static ASTNode * getter_clause() {
    return NULL;
}

static ASTNode * setter_clause() {
    return NULL;
}

static ASTNode * getter_setter_keyword_block() {
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

// function dec

static ASTNode * function_head() {
    return NULL;
}

static ASTNode * function_name() {
    if (tokens[cur]->type == IDENTIFIER)
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