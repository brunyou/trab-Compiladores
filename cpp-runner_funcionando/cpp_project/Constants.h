#ifndef CONSTANTS_H
#define CONSTANTS_H

enum TokenId 
{
    EPSILON  = 0,
    DOLLAR   = 1,
    t_MULTI_LINE_COMMENT = 2,
    t_SINGLE_LINE_COMMENT = 3,
    t_INTEGER = 4,
    t_FLOAT = 5,
    t_ID = 6,
    t_STRING = 7,
    t_INTEGER_KEYWORD = 8,
    t_FLOAT_KEYWORD = 9,
    t_STRING_KEYWORD = 10,
    t_BOOL_KEYWORD = 11,
    t_CHAR_KEYWORD = 12,
    t_DOUBLE_KEYWORD = 13,
    t_VOID_KEYWORD = 14,
    t_PROCEDURE_KEYWORD = 15,
    t_FUNCTION_KEYWORD = 16,
    t_IF_KEYWORD = 17,
    t_ELSE_KEYWORD = 18,
    t_FOR_KEYWORD = 19,
    t_WHILE_KEYWORD = 20,
    t_RETURN_KEYWORD = 21,
    t_SWITCH_KEYWORD = 22,
    t_CASE_KEYWORD = 23,
    t_DEFAULT_KEYWORD = 24,
    t_BREAK_KEYWORD = 25,
    t_CONTINUE_KEYWORD = 26,
    t_DO_KEYWORD = 27,
    t_LEIA_KEYWORD = 28,
    t_ESCREVA_KEYWORD = 29,
    t_OPERATOR_PLUS = 30,
    t_OPERATOR_MINUS = 31,
    t_OPERATOR_DIVIDE = 32,
    t_OPERATOR_MULTIPLY = 33,
    t_OPERATOR_SINGLE_EQUAL = 34,
    t_OPERATOR_MOD = 35,
    t_OPERATOR_GREATER = 36,
    t_OPERATOR_LESS = 37,
    t_OPERATOR_GREATER_EQUAL = 38,
    t_OPERATOR_LESS_EQUAL = 39,
    t_OPERATOR_EQUAL = 40,
    t_OPERATOR_NOT_EQUAL = 41,
    t_OPERATOR_AND = 42,
    t_OPERATOR_OR = 43,
    t_OPERATOR_NOT = 44,
    t_OPERATOR_SHIFT_LEFT = 45,
    t_OPERATOR_SHIFT_RIGHT = 46,
    t_OPERATOR_BIT_AND = 47,
    t_OPERATOR_BIT_OR = 48,
    t_OPERATOR_BIT_NOT = 49,
    t_OPERATOR_BIT_XOR = 50,
    t_DELIMITER_DOT = 51,
    t_DELIMITER_COMMA = 52,
    t_DELIMITER_SEMICOLON = 53,
    t_DELIMITER_COLON = 54,
    t_DELIMITER_LPAREN = 55,
    t_DELIMITER_RPAREN = 56,
    t_DELIMITER_LBRACKET = 57,
    t_DELIMITER_RBRACKET = 58,
    t_DELIMITER_LCURLY = 59,
    t_DELIMITER_RCURLY = 60
};

const int STATES_COUNT = 43;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern int SCANNER_CONTEXT[STATES_COUNT][2];

extern int SPECIAL_CASES_INDEXES[62];

extern const char *SPECIAL_CASES_KEYS[22];

extern int SPECIAL_CASES_VALUES[22];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 111;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[238][171][2];

extern const int PRODUCTIONS[98][2];

extern const char *PARSER_ERROR[238];

#endif
