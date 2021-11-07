#ifndef astnodetype_h
#define astnodetype_h

typedef enum {
    TOP_LEVEL_DECLARATION,
    NUMERIC_LITERAL_INT,
    NUMERIC_LITERAL_FLOAT,
    BOOLEAN_LITERAL,
    STRING_LITERAL,
    NIL_LITERAL,
    LITERAL,

    BINARY_OPERATOR,
    PREFIX_OPERATOR,
    POSTFIX_OPERATOR,

    TYPE_NAME,
    GENERIC_ARGUMENT,
    GENERIC_ARGUMENT_LIST,
    GENERIC_ARGUMENT_CLAUSE,
    IDENTIFIER_LIST,
    TYPE_IDENTIFIER,
    TYPE_ANNOTATION,
    ELEMENT_NAME,
    TUPLE_TYPE_ELEMENT,
    TUPLE_TYPE_ELEMENT_LIST,
    TUPLE_TYPE,
    ARGUMENT_LABEL,
    FUNCTION_TYPE_ARGUMENT,
    FUNCTION_TYPE_ARGUMENT_LIST,
    FUNCTION_TYPE_ARGUMENT_CLAUSE,
    FUNCTION_TYPE,
    ARRAY_TYPE,
    DICTIONARY_TYPE,
    OPTIONAL_TYPE,
    IMPLICITLY_UNWRAPPED_OPTIONAL_TYPE,
    PROTOCOL_COMPOSITION_CONTINUATION,
    PROTOCOL_COMPOSITION_TYPE,
    ANY_TYPE,
    TYPE_INHERITANCE_LIST,
    TYPE_INHERITANCE_CLAUSE,
    TYPE,

    TRY_OPERATOR,
    ARRAY_LITERAL,
    ARRAY_LITERAL_ITEM,
    ARRAY_LITERAL_ITEMS,
    DICTIONARY_LITERAL_ITEM,
    DICTIONARY_LITERAL_ITEMS,
    DICTIONARY_LITERAL,
    LITERAL_EXPRESSION,

    SELF_INITIALIZER_EXPRESSION,
    SELF_SUBSCRIPT_EXPRESSION,
    SELF_METHOD_EXPRESSION,
    SELF_EXPRESSION,

    SUPERCLASS_INITIALIZER_EXPRESSION,
    SUPERCLASS_SUBSCRIPT_EXPRESSION,
    SUPERCLASS_METHOD_EXPRESSION,
    SUPERCLASS_EXPRESSION,

    CLOSURE_PARAMETER_NAME,
    CLOSURE_PARAMETER,
    CLOSURE_PARAMETER_LIST,
    CLOSURE_PARAMETER_CLAUSE,
    FUNCTION_RESULT,
    CLOSURE_SIGNATURE,
    CLOSURE_EXPRESSION,

    PARENTHESIZED_EXPRESSION,
    TUPLE_ELEMENT,
    TUPLE_ELEMENT_LIST,
    TUPLE_EXPRESSION,
    WILDCARD_EXPRESSION,
    SELECTOR_EXPRESSION,
    PRIMARY_EXPRESSION,

    EXPLICIT_MEMBER_EXPRESSION,
    LABELED_TRAILING_CLOSURE,
    LABELED_TRAILING_CLOSURES,
    TRAILING_CLOSURES,
    FUNCTION_CALL_ARGUMENT,
    FUNCTION_CALL_ARGUMENT_LIST,
    FUNCTION_CALL_ARGUMENT_CLAUSE,
    FUNCTION_CALL_EXPRESSION,

    INITIALIZER_EXPRESSION,
    SUBSCRIPT_EXPRESSION,
    FORCED_VALUE_EXPRESSION,
    OPTIONAL_CHAINING_EXPRESSION,
    POSTFIX_EXPRESSION,
    PREFIX_EXPRESSION,

    TYPE_CASTING_OPERATOR,
    CONDITIONAL_OPERATOR,
    ASSIGNMENT_OPERATOR,

    BINARY_EXPRESSION,
    BINARY_EXPRESSIONS,
    EXPRESSION,
    EXPRESSION_LIST,

    STATEMENT,
    STATEMENTS,
    LOOP_STATEMENT,
    FOR_IN_STATEMENT,
    WHILE_STATEMENT,
    CONDITION_LIST,
    CONDITION,
    CASE_CONDITION,
    OPTIONAL_BINDING_CONDITION,
    REPEAT_WHILE_STATEMENT,
    BRANCH_STATEMENT,
    IF_STATEMENT,
    ELSE_CLAUSE,
    GUARD_STATEMENT,

    SWITCH_CASES,
    SWITCH_CASE,
    CASE_LABEL,
    CASE_ITEM_LIST,
    CASE_ITEM,
    DEFAULT_LABEL,

    WHERE_CLAUSE,
    WHERE_EXPRESSION,
    LABELED_STATEMENT,
    STATEMENT_LABEL,
    LABEL_NAME,

    CONTROL_TRANSFER_STATEMENT,
    BREAK_STATEMENT,
    CONTINUE_STATEMENT,
    FALLTHROUGH_STATEMENT,
    RETURN_STATEMENT,
    THROW_STATEMENT,

    DO_STATEMENT,
    CATCH_CLAUSES,
    CATCH_CLAUSE,
    CATCH_PATTERN_LIST,
    CATCH_PATTERN,

    DECLARATION,
    DECLARATIONS,
    CODE_BLOCK,
    CONSTANT_DECLARATION,
    PATTERN_INITIALIZER_LIST,
    PATTERN_INITIALIZER,
    INITIALIZER,
    TYPEDEF_DECLARATION,

    VARIABLE_NAME,
    VARIABLE_DECLARATION,
    VARIABLE_DECLARATION_HEAD,
    GETTER_SETTER_BLOCK,
    GETTER_CLAUSE,
    SETTER_CLAUSE,
    GETTER_SETTER_KEYWORD_BLOCK,
    WILLSET_DIDSET_BLOCK,
    WILLSET_CLAUSE,
    DIDSET_CLAUSE,

    FUNCTION_BODY,
    FUNCTION_DECLARATION,
    FUNCTION_HEAD,
    FUNCTION_NAME,
    FUNCTION_SIGNATURE,
    PARAMETER_CLAUSE,
    PARAMETER_LIST,
    PARAMETER,
    ACCESS_LEVEL_MODIFIER,

    DECLARATION_MODIFIERS,
    DECLARATION_MODIFIER,
    INITIALIZER_HEAD,
    INITIALIZER_BODY,
    INITIALIZER_DECLARATION,

    PATTERN,
    WILCARD_PATTERN,
    IDENTIFIER_PATTERN,
    TUPLE_PATTERN,
    VALUE_BINDING_PATTERN,
    ENUM_CASE_PATTERN,
    OPTIONAL_PATTERN,
    TYPE_CASTING_PATTERN,
    EXPRESION_PATTERN,

    CLASS_MEMBER,
    CLASS_MEMBERS,
    CLASS_BODY,
    CLASS_NAME,
    CLASS_DECLARATION,

    GENERIC_PARAMETER,
    GENERIC_PARAMETER_LIST,
    GENERIC_PARAMETER_CLAUSE
} ASTNodeType;

static char * ast_string_table[] = {
    "TOP_LEVEL_DECLARATION",
    "NUMERIC_LITERAL_INT",
    "NUMERIC_LITERAL_FLOAT",
    "BOOLEAN_LITERAL",
    "STRING_LITERAL",
    "NIL_LITERAL",
    "LITERAL",

    "BINARY_OPERATOR",
    "PREFIX_OPERATOR",
    "POSTFIX_OPERATOR",

    "TYPE_NAME",
    "GENERIC_ARGUMENT",
    "GENERIC_ARGUMENT_LIST",
    "GENERIC_ARGUMENT_CLAUSE",
    "IDENTIFIER_LIST",
    "TYPE_IDENTIFIER",
    "TYPE_ANNOTATION",
    "ELEMENT_NAME",
    "TUPLE_TYPE_ELEMENT",
    "TUPLE_TYPE_ELEMENT_LIST",
    "TUPLE_TYPE",
    "ARGUMENT_LABEL",
    "FUNCTION_TYPE_ARGUMENT",
    "FUNCTION_TYPE_ARGUMENT_LIST",
    "FUNCTION_TYPE_ARGUMENT_CLAUSE",
    "FUNCTION_TYPE",
    "ARRAY_TYPE",
    "DICTIONARY_TYPE",
    "OPTIONAL_TYPE",
    "IMPLICITLY_UNWRAPPED_OPTIONAL_TYPE",
    "PROTOCOL_COMPOSITION_CONTINUATION",
    "PROTOCOL_COMPOSITION_TYPE",
    "ANY_TYPE",
    "TYPE_INHERITANCE_LIST",
    "TYPE_INHERITANCE_CLAUSE",
    "TYPE",

    "TRY_OPERATOR",
    "ARRAY_LITERAL",
    "ARRAY_LITERAL_ITEM",
    "ARRAY_LITERAL_ITEMS",
    "DICTIONARY_LITERAL_ITEM",
    "DICTIONARY_LITERAL_ITEMS",
    "DICTIONARY_LITERAL",
    "LITERAL_EXPRESSION",

    "SELF_INITIALIZER_EXPRESSION",
    "SELF_SUBSCRIPT_EXPRESSION",
    "SELF_METHOD_EXPRESSION",
    "SELF_EXPRESSION",

    "SUPERCLASS_INITIALIZER_EXPRESSION",
    "SUPERCLASS_SUBSCRIPT_EXPRESSION",
    "SUPERCLASS_METHOD_EXPRESSION",
    "SUPERCLASS_EXPRESSION",

    "CLOSURE_PARAMETER_NAME",
    "CLOSURE_PARAMETER",
    "CLOSURE_PARAMETER_LIST",
    "CLOSURE_PARAMETER_CLAUSE",
    "FUNCTION_RESULT",
    "CLOSURE_SIGNATURE",
    "CLOSURE_EXPRESSION",

    "PARENTHESIZED_EXPRESSION",
    "TUPLE_ELEMENT",
    "TUPLE_ELEMENT_LIST",
    "TUPLE_EXPRESSION",
    "WILDCARD_EXPRESSION",
    "SELECTOR_EXPRESSION",
    "PRIMARY_EXPRESSION",

    "EXPLICIT_MEMBER_EXPRESSION",
    "LABELED_TRAILING_CLOSURE",
    "LABELED_TRAILING_CLOSURES",
    "TRAILING_CLOSURES",
    "FUNCTION_CALL_ARGUMENT",
    "FUNCTION_CALL_ARGUMENT_LIST",
    "FUNCTION_CALL_ARGUMENT_CLAUSE",
    "FUNCTION_CALL_EXPRESSION",

    "INITIALIZER_EXPRESSION",
    "SUBSCRIPT_EXPRESSION",
    "FORCED_VALUE_EXPRESSION",
    "OPTIONAL_CHAINING_EXPRESSION",
    "POSTFIX_EXPRESSION",
    "PREFIX_EXPRESSION",

    "TYPE_CASTING_OPERATOR",
    "CONDITIONAL_OPERATOR",
    "ASSIGNMENT_OPERATOR",

    "BINARY_EXPRESSION",
    "BINARY_EXPRESSIONS",
    "EXPRESSION",
    "EXPRESSION_LIST",

    "STATEMENT",
    "STATEMENTS",
    "LOOP_STATEMENT",
    "FOR_IN_STATEMENT",
    "WHILE_STATEMENT",
    "CONDITION_LIST",
    "CONDITION",
    "CASE_CONDITION",
    "OPTIONAL_BINDING_CONDITION",
    "REPEAT_WHILE_STATEMENT",
    "BRANCH_STATEMENT",
    "IF_STATEMENT",
    "ELSE_CLAUSE",
    "GUARD_STATEMENT",

    "SWITCH_CASES",
    "SWITCH_CASE",
    "CASE_LABEL",
    "CASE_ITEM_LIST",
    "CASE_ITEM",
    "DEFAULT_LABEL",

    "WHERE_CLAUSE",
    "WHERE_EXPRESSION",
    "LABELED_STATEMENT",
    "STATEMENT_LABEL",
    "LABEL_NAME",

    "CONTROL_TRANSFER_STATEMENT",
    "BREAK_STATEMENT",
    "CONTINUE_STATEMENT",
    "FALLTHROUGH_STATEMENT",
    "RETURN_STATEMENT",
    "THROW_STATEMENT",

    "DO_STATEMENT",
    "CATCH_CLAUSES",
    "CATCH_CLAUSE",
    "CATCH_PATTERN_LIST",
    "CATCH_PATTERN",

    "DECLARATION",
    "DECLARATIONS",
    "CODE_BLOCK",
    "CONSTANT_DECLARATION",
    "PATTERN_INITIALIZER_LIST",
    "PATTERN_INITIALIZER",
    "INITIALIZER",
    "TYPEDEF_DECLARATION",

    "VARIABLE_NAME",
    "VARIABLE_DECLARATION",
    "VARIABLE_DECLARATION_HEAD",
    "GETTER_SETTER_BLOCK",
    "GETTER_CLAUSE",
    "SETTER_CLAUSE",
    "GETTER_SETTER_KEYWORD_BLOCK",
    "WILLSET_DIDSET_BLOCK",
    "WILLSET_CLAUSE",
    "DIDSET_CLAUSE",

    "FUNCTION_BODY",
    "FUNCTION_DECLARATION",
    "FUNCTION_HEAD",
    "FUNCTION_NAME",
    "FUNCTION_SIGNATURE",
    "PARAMETER_CLAUSE",
    "PARAMETER_LIST",
    "PARAMETER",
    "ACCESS_LEVEL_MODIFIER",

    "DECLARATION_MODIFIERS",
    "DECLARATION_MODIFIER",
    "INITIALIZER_HEAD",
    "INITIALIZER_BODY",
    "INITIALIZER_DECLARATION",

    "PATTERN",
    "WILCARD_PATTERN",
    "IDENTIFIER_PATTERN",
    "TUPLE_PATTERN",
    "VALUE_BINDING_PATTERN",
    "ENUM_CASE_PATTERN",
    "OPTIONAL_PATTERN",
    "TYPE_CASTING_PATTERN",
    "EXPRESION_PATTERN",

    "CLASS_MEMBER",
    "CLASS_MEMBERS",
    "CLASS_BODY",
    "CLASS_NAME",
    "CLASS_DECLARATION",

    "GENERIC_PARAMETER",
    "GENERIC_PARAMETER_LIST",
    "GENERIC_PARAMETER_CLAUSE"
};

static char * node_type_to_string(ASTNodeType t) {
    return ast_string_table[t];
}

#endif
#pragma once