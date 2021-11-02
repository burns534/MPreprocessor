#include "v2parser.h"

#define CURSOR_STACK_DEPTH 4096
#define CHILD_BUFFER_SIZE 4096
#define ARGUMENT_LIST_MAX 32

static Token **tokens;
static size_t tc, bi = 0, cur = 0;
static map *primitive_types, *type_names;
static size_t cursor_stack[4096], cursor_stack_top = 0;
static ASTNode *buf[CHILD_BUFFER_SIZE];

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

static ASTNode * create_node(ASTNodeType type) {
    ASTNode *result = malloc(sizeof(ASTNode));
    result->child_count = 0;
    result->type = type;
    return result;
}

static ASTNode * create_node_withvalue(ASTNodeType type, char *v) {
    ASTNode *result = malloc(sizeof(ASTNode));
    result->child_count = 0;
    result->type = type;
    result->value = v;
    return result;
}


static inline void set_children(ASTNode *n) {
    n->children = malloc(bi * sizeof(ASTNode *));
    n->child_count = bi;
    bi = 0;
    for (int i = 0; i < bi; i++)
        n->children[i] = buf[i];
}

static inline void set_value(char *s, ASTNode *n) {
    size_t len = strlen(s);
    n->value = malloc(len + 1);
    strcpy(n->value, s);
    n->value[len] = 0;
}

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

// make the lexer do this
static ASTNode * floating_point_literal() {
    return NULL;
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
    if (tokens[cur++]->type == NIL)
        return create_node(NIL_LITERAL);
    cur--;
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
    if ((buf[0] = numeric_literal())
    || (buf[0] = string_literal())
    || (buf[0] = boolean_literal())
    || (buf[0] = nil_literal())) {
        ASTNode *result = create_node(LITERAL);
        bi++;
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * binary_operator() {
    if (tokens[cur]->type == OPERATOR) {
        ASTNode *result = create_node(BINARY_OPERATOR);
        result->value = tokens[cur++]->value;
        return result;
    }
    return NULL;
}

static ASTNode * prefix_operator() {
    if (tokens[cur]->type == OPERATOR) {
        ASTNode *result = create_node(PREFIX_OPERATOR);
        result->value = tokens[cur++]->value;
        return result;
    }
    return NULL;
}

static ASTNode * postfix_operator() {
    if (tokens[cur]->type == OPERATOR) {
        ASTNode *result = create_node(POSTFIX_OPERATOR);
        result->value = tokens[cur++]->value;
        return result;
    }
    return NULL;
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

static ASTNode * type_name() {
    if (tokens[cur]->type == IDENTIFIER && (primitive_type() || map_contains(tokens[cur]->value, type_names))) {
        ASTNode *result = create_node(TYPE_NAME);
        result->value = tokens[cur++]->value;
        return result;
    }
    return NULL;
}

// can't be empty
// operator stores < and >
static ASTNode * generic_argument_clause() {
    if (tokens[cur]->type == OPERATOR && strcmp(tokens[cur]->value, "<") == 0 && push()) {
        ASTNode *result = NULL;
        cur++;
        while(cur < tc && (buf[bi] = type())) {
            bi++;
            cur++;
            if (cur < tc && tokens[cur]->type == COMMA) {
                cur++;
                continue;
            } else {
                break;
            }
        }

        if (cur >= tc) // this might not be right
            error("eof reached inside generic argument clause");
            
        if (strcmp(tokens[cur]->value, ">") != 0)
            error("expected >");

        if (bi > 0) { // we got arguments
            result = create_node(GENERIC_ARGUMENT_CLAUSE);
            set_children(result);
        }

        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * type_identifier() {
    if (push() && (buf[0] = type_name())) {
        ASTNode *result = create_node(TYPE_IDENTIFIER);
        if (push() && (buf[++bi] = generic_argument_clause())) {
            if (cur < tc && tokens[cur]->type == OPERATOR && strcmp(tokens[cur]->value, ".") == 0) {
                if (!(buf[++bi] = type_identifier()))
                    error("expected type identifier following .");
            }
            // case 1 and 2
            pop();
        } else if (cur < tc && tokens[cur]->type == OPERATOR && strcmp(tokens[cur]->value, ".") == 0) {
            // case 3
            if (!(buf[++bi] = type_identifier()))
                error("expected type identifier following .");
        } else {
            pop();
        }

        // case 4
        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * type_annotation() {
    if (push() && tokens[cur]->type == COLON) {
        cur++;
        if (!(buf[0] = type()))
            error("expected type following :");
        ASTNode *result = create_node(TYPE_ANNOTATION);
        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * tuple_type() {
    if (push() && tokens[cur]->type == OPEN_PAREN) {
        ASTNode *result = NULL;
        cur++;
        if (push() && (buf[0] = type())) {
            bi++;
            while(cur < tc && tokens[cur]->type == COMMA) {
                cur++;
                if (!(buf[bi++] = type()))
                    error("expected type following ,");
            }

            pop();
        } else {
            if (tokens[cur]->type != CLOSE_PAREN)
                error("expected )");
        }
        result = create_node(TUPLE_TYPE);
        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * function_type_argument() {
    if (push() && (buf[0] == type())) {
        bi++;
        ASTNode *result = create_node(FUNCTION_TYPE_ARGUMENT);
        set_children(result);
        pop();
        return result;
    } else if (tokens[cur++]->type == IDENTIFIER && (buf[0] = type_annotation())) {
        bi++;
        ASTNode *result = create_node(FUNCTION_TYPE_ARGUMENT);
        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * function_type_argument_list() {
    if (push() && (buf[0] = function_type_argument())) {
        bi++;
        while(cur < tc && tokens[cur]->type == COMMA) {
            cur++;
            if (!(buf[bi++] = function_type_argument()))
                error("expected function type argument");
        }
        ASTNode *result = create_node(FUNCTION_TYPE_ARGUMENT_LIST);
        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * function_type_argument_clause() {
    if (tokens[cur]->type == OPEN_PAREN) {
        cur++;
        ASTNode *result = create_node(FUNCTION_TYPE_ARGUMENT_CLAUSE);
        if (tokens[cur]->type == CLOSE_PAREN) {
            cur++;
            return result;
        } else if (push() && (buf[0] = function_type_argument())) {
            bi++;
            if (cur + 2 < tc && tokens[cur]->type == OPERATOR && strcmp(tokens[cur]->value, ".") == 0
            && tokens[cur + 1]->type == OPERATOR && strcmp(tokens[cur + 1]->value, ".") == 0
            && tokens[cur + 2]->type == OPERATOR && strcmp(tokens[cur + 2]->value, ".") == 0) {
                // case 2 variadic
                cur += 3;
                result->value = "...";
            }

            if (cur < tc && tokens[cur]->type != CLOSE_PAREN)
                error("expected parenthesis");
            
            cur++;

            set_children(result);
            pop();
            return result;
        } 
        pop_set();
        return NULL;
    }
    return NULL;
}

static ASTNode * function_type() {
    if (push() && (buf[0] == function_type_argument_clause())) {
        ASTNode *result = create_node(FUNCTION_TYPE);
        bi++;
        if (tokens[cur]->type == THROWS) {
            result->value = "throws";
            cur++;
        }

        if (cur + 1 >= tc)
            error("eof during function type");
        if (tokens[cur++]->type != RETURN_ANNOTATION)
            error("expected return annotation");
        if (!(buf[bi++] = type()))
            error("expected return type");

        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * array_type() {
    if (push() && tokens[cur++]->type == OPEN_SQ_BRACE && (buf[0] = type()) && tokens[cur]->type == CLOSE_SQ_BRACE) {
        cur++;
        ASTNode *result = create_node(ARRAY_TYPE);
        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * dictionary_type() {
    if (push() && tokens[cur++]->type == OPEN_SQ_BRACE && (buf[0] = type()) && tokens[cur]->type == COLON) {
        cur++;
        bi++;
        if (!(buf[bi++] = type()))
            error("expected type after colon");
        if (cur >= tc)
            error("eof in dictioary type");
        if (tokens[cur++]->type != CLOSE_SQ_BRACE)
            error("expected close brace after type");
        
        ASTNode *result = create_node(DICTIONARY_TYPE);
        set_children(result);
        pop();
        return result;
    }
    pop_set();
    return NULL;
}

static ASTNode * optional_type() {
    if (push() && (buf[0] = type()) && cur < tc 
    && tokens[cur]->type == OPERATOR 
    && strcmp(tokens[cur++]->value, "!") == 0) {
        ASTNode *result = create_node(OPTIONAL_TYPE);
        bi++;
        set_children(result);
        return result;
    }
    pop_set();
    return NULL;    
}

static ASTNode * implicitly_unwrapped_optional_type() {
    if (push() && (buf[0] = type()) && cur < tc 
    && tokens[cur]->type == OPERATOR 
    && strcmp(tokens[cur++]->value, "?") == 0) {
        ASTNode *result = create_node(IMPLICITLY_UNWRAPPED_OPTIONAL_TYPE);
        bi++;
        set_children(result);
        return result;
    }
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
    if (tokens[cur]->type == ANY) {
        ASTNode *result = create_node(ANY_TYPE);
        cur++;
        return result;
    }
}

static ASTNode * type_inheritance_list() {
    return NULL;
}

static ASTNode * type_inheritance_clause() {
    return NULL;
}

ASTNode * type() {
    if ((buf[0] = function_type())
    || (buf[0] = array_type())
    || (buf[0] = dictionary_type())
    || (buf[0] = type_identifier())
    || (buf[0] = tuple_type())
    || (buf[0] = optional_type())
    || (buf[0] = implicitly_unwrapped_optional_type())
    || (buf[0] = protocol_composition_type())
    || (buf[0] = any_type())) {
        ASTNode *result = create_node(TYPE);
        bi++;
        set_children(result);
        return result;
    } else if (push() && tokens[cur]->type == OPEN_PAREN) {
        // not sure if should throw error here or not..
        if ((buf[0] = type()) && tokens[cur]->type == CLOSE_PAREN) {
            bi++;
            cur++;
            ASTNode *result = create_node(TYPE);
            result->value = "parenthesized";
            set_children(result);
            return result;
        }
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
            return create_node_withvalue(DECLARATION_MODIFIER, tokens[cur++]->value);
            break;
        default:
            break;
    }

    if ((buf[0] = access_level_modifier())) {
        bi++;
        ASTNode *result = create_node(DECLARATION_MODIFIER);
        set_children(result);
        return result;
    }

    return NULL;
}

static ASTNode * declaration_modifiers() {
    if ((buf[0] = declaration())) {
        bi++;
        ASTNode *result = create_node(DECLARATION_MODIFIERS);
        while((buf[bi] = declaration())) bi++;
        set_children(result);
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
    if ((buf[0] = expression())) {
        bi++;
        ASTNode *result = create_node(ARRAY_LITERAL_ITEM);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * array_literal_items() {
    if ((buf[0] = array_literal_item())) {
        bi++;
        ASTNode *result = create_node(ARRAY_LITERAL_ITEMS);

        while(cur < tc && tokens[cur]->type == COMMA) {
            cur++;
            if (!(buf[bi++] = array_literal_item()))
                error("expected array literal item following comma");
        }

        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * array_literal() {
    if (push() && tokens[cur]->type == OPEN_SQ_BRACE) {
        cur++;
        if ((buf[0] = array_literal_items())) {
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
    if (push() && (buf[0] = expression())) {
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
    if ((buf[0] = dictionary_literal_item())) {
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
        if ((buf[0] = dictionary_literal_items())) {
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
    if ((buf[0] = literal())
    || (buf[0] = array_literal()) 
    || (buf[0] = dictionary_literal())) {
        bi = 1;
        ASTNode *result = create_node(LITERAL_EXPRESSION);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * self_initializer_expression() {
    if (push() && tokens[cur++]->type == SELF && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, ".") && tokens[cur++]->type == INIT && pop())
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
    if ((buf[0] = self_method_expression())
    || (buf[0] = self_subscript_expression())
    || (buf[0] = self_initializer_expression())) {
        bi = 1;
        ASTNode *result = create_node(SELF_EXPRESSION);
        set_children(result);
        return result;
    }
    if (tokens[cur]->type == SELF)
        return create_node_withvalue(SELF_EXPRESSION, tokens[cur++]->value);
    return NULL;
}

static ASTNode * superclass_initializer_expression() {
    if (push() && tokens[cur++]->type == SUPER && tokens[cur]->type == OPERATOR && strcmp(tokens[cur++]->value, ".") && tokens[cur++]->type == INIT && pop())
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
    if ((buf[0] = superclass_method_expression())
    || (buf[0] = superclass_subscript_expression())
    || (buf[0] = supeclass_initializer_expression())) {
        bi = 1;
        ASTNode *result = create_node(SUPERCLASS_EXPRESSION);
        set_children(result);
        return result;
    }
    if (tokens[cur]->type == SUPER)
        return create_node_withvalue(SUPERCLASS_EXPRESSION, tokens[cur++]->value);
    return NULL;
}

// closures

static ASTNode * closure_parameter_name() {
    if (tokens[cur]->type == IDENTIFIER) 
        return create_node_withvalue(CLOSURE_PARAMETER_NAME, tokens[cur++]->value);
    return NULL;
}

static ASTNode * closure_parameter() {
    return NULL;
}

static ASTNode * closure_parameter_list() {
    return NULL;
}

static ASTNode * closure_parameter_clause() {
    return NULL;
}

static ASTNode * function_result() {
    if ((buf[0] = type())) {
        bi++;
        ASTNode *result = create_node(FUNCTION_RESULT);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * closure_signature() {
    return NULL;
}

static ASTNode * closure_expression() {
    return NULL;
}

// other expressions

static ASTNode * parenthesized_expression() {
    return NULL;
}

static ASTNode * tuple_element() {
    return NULL;
}

static ASTNode * tuple_expression() {
    return NULL;
}

static ASTNode * wildcard_expression() {
    if (tokens[cur]->type == WILDCARD) 
        return create_node_withvalue(WILDCARD_EXPRESSION, tokens[cur++]->value);
    return NULL;
}

static ASTNode * selector_expression() {
    return NULL;
}

static ASTNode * primary_expression() {
    if (tokens[cur]->type == IDENTIFIER) {
        ASTNode *result = create_node_withvalue(PRIMARY_EXPRESSION, tokens[cur++]->value);
        if ((buf[0] = generic_argument_clause())) {
            bi++;
            set_children(result);
        }
        return result;
    } else if ((buf[0] = literal_expression())
    || (buf[0] = self_expression())
    || (buf[0] = superclass_expression())
    || (buf[0] = closure_expression())
    || (buf[0] = parenthesized_expression())
    || (buf[0] = tuple_expression())
    || (buf[0] = wildcard_expression())
    || (buf[0] = selector_expression()))
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

static ASTNode * function_call_argument() {
    return NULL;
}

static ASTNode * function_argument_list() {
    return NULL;
}

static ASTNode * function_argument_clause() {
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
    return NULL;
}

static ASTNode * prefix_expression() {
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
    return NULL;
}

static ASTNode * expression() {

    return NULL;
}

static ASTNode * expression_list() {
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

static ASTNode * condition_list() {
    return NULL;
}

static ASTNode * condition() {
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
    return NULL;
}

ASTNode * statement() {
    if ((buf[0] = expression())
    || (buf[0] = declaration())
    || (buf[0] = loop_statement())
    || (buf[0] = branch_statement())
    || (buf[0] = labeled_statement())
    || (buf[0] = control_transfer_statement())
    || (buf[0] = do_statement())) {
        bi++;
        ASTNode *result = create_node(STATEMENT);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * statements() {
    if ((buf[0] = statement())) {
        ASTNode *result = create_node(STATEMENTS);
        bi++;
        while((buf[bi] = statement())) bi++;
        set_children(result);
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

static ASTNode * pattern_initializer() {
    if ((buf[0] = pattern())) {
        bi++;
        ASTNode *result = create_node(PATTERN_INITIALIZER);
        if ((buf[bi] = initializer())) bi++;
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * pattern_initializer_list() {
    if ((buf[0] = pattern_initializer())) {
        bi++;
        ASTNode *result = create_node(PATTERN_INITIALIZER);
        while(cur < tc && tokens[cur]->type == COMMA) {
            cur++;
            if (!(buf[bi++] = pattern_initializer()))
                error("expected pattern initializer following comma");
        }
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * initializer() {
    if ((buf[0] = expression())) {
        bi++;
        ASTNode *result = create_node(INITIALIZER);
        set_children(result);
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
        return create_node_withvalue(FUNCTION_NAME, tokens[cur++]->value);
    return NULL;
}

static ASTNode * function_signature() {
    return NULL;
}


static ASTNode * parameter() {
    return NULL;
}

static ASTNode * parameter_list() {
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
    if ((buf[0] = constant_declaration())
    || (buf[0] = variable_declaration())
    || (buf[0] = typedef_declaration())
    || (buf[0] = function_declaration())
    || (buf[0] = enum_declaration())
    || (buf[0] = struct_declaration())
    || (buf[0] = class_declaration())
    || (buf[0] = protocol_declaration())
    || (buf[0] = initializer_declaration())
    || (buf[0] = deinitializer_declaration())
    || (buf[0] = extension_declaration())
    || (buf[0] = subscript_declaration())
    || (buf[0] = operator_declaration())
    || (buf[0] = precedence_group_declaration())) {
        ASTNode *result = create_node(DECLARATION);
        set_children(result);
        return result;
    }
    return NULL;
}

static ASTNode * declarations() {
    if ((buf[0] = declaration())) {
        ASTNode *result = create_node(DECLARATIONS);
        bi++;
        while((buf[bi] = declaration())) bi++;
        set_children(result);
        return result;
    }
    return NULL;
}



static ASTNode * access_level_modifier() {
    if (tokens[cur]->type == PRIVATE || tokens[cur]->type == PUBLIC)
        return create_node_withvalue(ACCESS_LEVEL_MODIFIER, tokens[cur++]->value);
    return NULL;
}

static ASTNode * top_level_declaration() {
    ASTNode *result = create_node(TOP_LEVEL_DECLARATION);
    if ((buf[0] = statements())) {
        bi++;
        set_children(result);
    }
    return result;
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