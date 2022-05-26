#ifndef TOKENIZER_H
#define TOKENIZER_H

#define TOK_ENUMS       \
    X(IF)               \
    X(ELSE)             \
    X(DO)               \
    X(WHILE)            \
    X(FOR)              \
    X(SWITCH)           \
    X(CASE)             \
    X(BREAK)            \
    X(CONTINUE)         \
    X(DEFAULT)          \
    X(RETURN)           \
    X(GOTO)             \
    X(TYPEDEF)          \
    X(STRUCT)           \
    X(UNION)            \
    X(ENUM)             \
    X(SIGNED)           \
    X(UNSIGNED)         \
    X(VOID)             \
    X(INT)              \
    X(CHAR)             \
    X(LONG)             \
    X(SHORT)            \
    X(FLOAT)            \
    X(DOUBLE)           \
    X(CONST)            \
    X(STATIC)           \
    X(EXTERN)           \
    X(AUTO)             \
    X(VOLATILE)         \
    X(REGISTER)         \
    X(RESTRICT)         \
    X(INLINE)           \
    X(IDENT)            \
    X(LITERAL_INT)      \
    X(LITERAL_FLOAT)    \
    X(LITERAL_CHAR)     \
    X(LITERAL_STRING)   \
    X(PRE_INCR)         \
    X(POST_INCR)        \
    X(PRE_DEC)          \
    X(POST_DEC)         \
    X(ARROW)            \
    X(SIZEOF)           \
    X(SH_LEFT)          \
    X(SH_RIGHT)         \
    X(LTE)              \
    X(GTE)              \
    X(EQ)               \
    X(NE)               \
    X(LOG_AND)          \
    X(LOG_OR)           \
    X(PLUS_EQ)          \
    X(MINUS_EQ)         \
    X(TIMES_EQ)         \
    X(DIV_EQ)           \
    X(MOD_EQ)           \
    X(AND_EQ)           \
    X(OR_EQ)            \
    X(XOR_EQ)           \
    X(SH_LEFT_EQ)       \
    X(SH_RIGHT_EQ)      \
    X(INVALID)

// TODO: merge token_type_t and ast_node_type_t?
#define X(A) TOK_ ## A,
typedef enum {
    TOK_EOF = 256,
    TOK_ENUMS
} token_type_t;
#undef X

// TODO: determine if pointer is in the heap or .rodata
typedef struct {
    token_type_t typ;
    union {
        const char * c_s;
        char * s;
        int i;
    } u;
} token_t;

typedef struct {
    int token_idx;
    int line_num;
} tokenizer_state_t;

extern tokenizer_state_t tok_state;

token_t peek_token(int n);
token_t get_token();

#endif /* TOKENIZER_H */
