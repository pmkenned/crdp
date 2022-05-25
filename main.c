#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

typedef struct zone_t zone_t;

static zone_t zone;

struct zone_t {
    void * base;
    size_t cap;
    uintptr_t ptr;
    zone_t * next_zone;
};

static int num_zone_init = 0;

static zone_t
zone_init(size_t cap)
{
    num_zone_init++;
    assert(cap > 0);
    zone_t z;
    z.cap = cap;
    z.base = malloc(cap);
    assert(z.base);
    z.ptr = (uintptr_t) z.base;
    z.next_zone = NULL;
    return z;
}

static void
zone_deinit(zone_t * zp)
{
    if (zp->next_zone) {
        zone_deinit(zp->next_zone);
    }
    free(zp->base);
}

static void *
zone_alloc_align(zone_t * zp, size_t sz, size_t align)
{
    zp->ptr += (zp->ptr % align == 0) ? 0 : align - (zp->ptr % align);
    void * old_p = (void *) zp->ptr;
    if (zp->ptr + sz <= (uintptr_t) zp->base + zp->cap) {
        zp->ptr += sz;
        return old_p;
    }
    // this zone is full
    size_t new_cap = sz > (1 << 20) ? sz : (1 << 20);
    zone_t * new_zone = malloc(sizeof(*new_zone));
    *new_zone = *zp;
    *zp = zone_init(new_cap);
    zp->next_zone = new_zone;
    return zone_alloc_align(zp, sz, align);
}

static void *
zone_alloc(zone_t * zp, size_t sz)
{
    size_t align = sz > 16 ? 16 : sz;
    return zone_alloc_align(zp, sz, align);
}

static void *
zone_realloc_align(zone_t * zp, void * ptr, size_t old_sz, size_t new_sz, size_t align)
{
    void * new_p = zone_alloc_align(zp, new_sz, align);
    memcpy(new_p, ptr, old_sz);
    return new_p;
}

#if 0
// TODO: is it possible to free the most recent allocation?
static void *
zone_free(zone_t * zp, void * ptr)
{
}
#endif

#define TOK_ENUMS           \
    X(TOK_IF)               \
    X(TOK_ELSE)             \
    X(TOK_DO)               \
    X(TOK_WHILE)            \
    X(TOK_FOR)              \
    X(TOK_SWITCH)           \
    X(TOK_CASE)             \
    X(TOK_BREAK)            \
    X(TOK_CONTINUE)         \
    X(TOK_DEFAULT)          \
    X(TOK_RETURN)           \
    X(TOK_GOTO)             \
    X(TOK_TYPEDEF)          \
    X(TOK_STRUCT)           \
    X(TOK_UNION)            \
    X(TOK_ENUM)             \
    X(TOK_SIGNED)           \
    X(TOK_UNSIGNED)         \
    X(TOK_VOID)             \
    X(TOK_INT)              \
    X(TOK_CHAR)             \
    X(TOK_LONG)             \
    X(TOK_SHORT)            \
    X(TOK_FLOAT)            \
    X(TOK_DOUBLE)           \
    X(TOK_CONST)            \
    X(TOK_STATIC)           \
    X(TOK_EXTERN)           \
    X(TOK_AUTO)             \
    X(TOK_VOLATILE)         \
    X(TOK_REGISTER)         \
    X(TOK_RESTRICT)         \
    X(TOK_INLINE)           \
    X(TOK_IDENT)            \
    X(TOK_LITERAL_INT)      \
    X(TOK_LITERAL_FLOAT)    \
    X(TOK_LITERAL_CHAR)     \
    X(TOK_LITERAL_STRING)   \
    X(TOK_PRE_INCR)         \
    X(TOK_POST_INCR)        \
    X(TOK_PRE_DEC)          \
    X(TOK_POST_DEC)         \
    X(TOK_ARROW)            \
    X(TOK_SIZEOF)           \
    X(TOK_SH_LEFT)          \
    X(TOK_SH_RIGHT)         \
    X(TOK_LTE)              \
    X(TOK_GTE)              \
    X(TOK_EQ)               \
    X(TOK_NE)               \
    X(TOK_LOG_AND)          \
    X(TOK_LOG_OR)           \
    X(TOK_PLUS_EQ)          \
    X(TOK_MINUS_EQ)         \
    X(TOK_TIMES_EQ)         \
    X(TOK_DIV_EQ)           \
    X(TOK_MOD_EQ)           \
    X(TOK_AND_EQ)           \
    X(TOK_OR_EQ)            \
    X(TOK_XOR_EQ)           \
    X(TOK_SH_LEFT_EQ)       \
    X(TOK_SH_RIGHT_EQ)      \
    X(TOK_INVALID)

// TODO: merge token_type_t and ast_node_type_t?
#define X(A) A,
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

static tokenizer_state_t tok_state = { 0, 1 };

static token_t token_list[] = {

    // extern char c;
    { .typ = TOK_EXTERN,                },
    { .typ = TOK_CHAR                   },
    { .typ = TOK_IDENT, .u.c_s = "c"    },
    { .typ = ';',                       },
    { .typ = '\n',                      },

    // static const signed char * const c;
    { .typ = TOK_STATIC,                },
    { .typ = TOK_CONST,                 },
    { .typ = TOK_SIGNED                 },
    { .typ = TOK_CHAR                   },
    { .typ = '*'                        },
    { .typ = TOK_CONST,                 },
    { .typ = TOK_IDENT, .u.c_s = "c"    },
    { .typ = ';',                       },
    { .typ = '\n',                      },

    // int x;
    { .typ = TOK_INT,                   },
    { .typ = TOK_IDENT, .u.c_s = "x"    },
    { .typ = ';',                       },
    { .typ = '\n',                      },

    // int f();
    { .typ = TOK_INT,                   },
    { .typ = TOK_IDENT, .u.c_s = "f"    },
    { .typ = '(',                       },
    { .typ = ')',                       },
    { .typ = ';',                       },
    { .typ = '\n',                      },

    // int f(int a);
    { .typ = TOK_INT,                   },
    { .typ = TOK_IDENT, .u.c_s = "f"    },
    { .typ = '(',                       },
    { .typ = TOK_INT,                   },
    { .typ = TOK_IDENT, .u.c_s = "a"    },
    { .typ = ')',                       },
    { .typ = ';',                       },
    { .typ = '\n',                      },

    // const int * f(int a, const char * b);
    { .typ = TOK_CONST,                 },
    { .typ = TOK_INT,                   },
    { .typ = '*',                       },
    { .typ = TOK_IDENT, .u.c_s = "f"    },
    { .typ = '(',                       },
    { .typ = TOK_INT,                   },
    { .typ = TOK_IDENT, .u.c_s = "a"    },
    { .typ = ',',                       },
    { .typ = TOK_CONST,                 },
    { .typ = TOK_CHAR,                  },
    { .typ = '*',                       },
    { .typ = TOK_IDENT, .u.c_s = "b"    },
    { .typ = ')',                       },
    { .typ = ';',                       },
    { .typ = '\n',                      },

    // int f() { return 0; }
    { .typ = TOK_INT,                   },
    { .typ = TOK_IDENT, .u.c_s = "f"    },
    { .typ = '(',                       },
    { .typ = ')',                       },
    { .typ = '{',                       },
    { .typ = TOK_RETURN,                },
    { .typ = TOK_LITERAL_INT, .u.i = 0  },
    { .typ = ';',                       },
    { .typ = '}',                       },
    { .typ = '\n',                      },

    { .typ = TOK_EOF,                   },
};

static token_t
peek_token(int n)
{
    token_t t;
    int peek_idx = tok_state.token_idx;
    for (int i = 0; i < n; i++) {
        do {
            t = token_list[peek_idx++];
            if (t.typ == TOK_EOF) {
                return t;
            }
        } while (t.typ == '\n');
    }
    return t;
}

#define AST_ENUMS       \
    X(AST_TU)           \
    X(AST_TYPE)         \
    X(AST_CAST)         \
    X(AST_VAR_DECL)     \
    X(AST_VAR_DEF)      \
    X(AST_FUNC_DECL)    \
    X(AST_FUNC_DEF)     \
    X(AST_STRUCT_DECL)  \
    X(AST_STRUCT_DEF)   \
    X(AST_UNION_DECL)   \
    X(AST_UNION_DEF)    \
    X(AST_ENUM_DECL)    \
    X(AST_ENUM_DEF)     \
    X(AST_TYPEDEF)      \
    X(AST_PARAM_LIST)   \
    X(AST_STMT)         \
    X(AST_EXPR)

#define X(A) A,
typedef enum {
    AST_INVALID = TOK_INVALID,
    AST_ENUMS
} ast_node_type_t;
#undef X

#define X(A) [A] = #A,
static const char * enum_strs[] = {
    TOK_ENUMS
    AST_ENUMS
};
#undef X

static token_t
get_token()
{
    token_t t;
    do {
        t = token_list[tok_state.token_idx++];
#if 0
        const char * enum_str = enum_strs[t.typ];
        if (enum_str == NULL) {
            if (isprint(t.typ))
                fprintf(stderr, " '%c'", t.typ);
        } else {
            fprintf(stderr, " %s", enum_str);
        }
        fprintf(stderr, "\n");
#endif
        if (t.typ == '\n') {
            tok_state.line_num++;
        }
    } while (t.typ == '\n');
    return t;
}

// TODO: unget newline
#if 0
static void
unget_token()
{
    tok_state.token_idx--;
#if 1
    int c = token_list[tok_state.token_idx].typ;
    const char * enum_str = enum_strs[c];
    if (enum_str == NULL) {
        if (isprint(c))
            fprintf(stderr, " '%c'", c);
    } else {
        fprintf(stderr, " %s", enum_str);
    }
    fprintf(stderr, "\n");
#endif
}
#endif

typedef struct ast_node_t ast_node_t;

struct ast_node_t {
    ast_node_type_t typ;
    char * s;
    int line_num;
    ast_node_t ** children;
    size_t num_children;
    size_t children_cap;
};

static void
ast_print_depth(ast_node_t * ast, FILE * fp, int depth)
{
    assert(ast);

    // indent
    for (int i = 0; i < depth; i++) {
        fputs("  ", fp);
    }

    fprintf(fp, "%d: ", ast->line_num);
    const char * enum_str = enum_strs[ast->typ];
    if (enum_str == NULL) {
        if (isprint(ast->typ))
            fprintf(fp, "%c", ast->typ);
    } else {
        fprintf(fp, "%s", enum_str);
    }
    if (ast->typ == (ast_node_type_t) TOK_IDENT) {
        fprintf(fp, " (%s)", ast->s);
    }
    fprintf(fp, "\n");
    for (size_t i = 0; i < ast->num_children; i++) {
        ast_print_depth(ast->children[i], fp, depth+1);
    }
}

static void
ast_print(ast_node_t * ast, FILE * fp)
{
    assert(ast);
    ast_print_depth(ast, fp, 0);
}

static void
ast_node_init_type_cap(ast_node_t * np, ast_node_type_t typ, size_t cap)
{
    //assert(cap > 0);
    np->typ = typ;
    np->s = NULL;
    np->line_num = tok_state.line_num;
    np->num_children = 0;
    np->children_cap = cap;
    np->children = NULL;
    if (np->children_cap > 0) {
        np->children = zone_alloc(&zone, sizeof(*np->children)*cap);
    }
}

static void
ast_node_init_type(ast_node_t * np, ast_node_type_t typ)
{
    ast_node_init_type_cap(np, typ, 4);
}

#if 0
static void
ast_node_init(ast_node_t * np)
{
    ast_node_init_type_cap(np, AST_INVALID, 4);
}
#endif

static void
ast_node_append_child(ast_node_t * np, ast_node_t * cp)
{
    if (np->num_children >= np->children_cap) {
        if (np->children_cap == 0) {
            size_t cap = (np->children_cap = 2);
            np->children = zone_alloc_align(&zone, sizeof(*np->children)*cap, 8);
        } else {
            size_t old_sz = sizeof(*np->children)*np->children_cap;
            size_t new_sz = sizeof(*np->children)*(np->children_cap *= 2);
            np->children = zone_realloc_align(&zone, np->children, old_sz, new_sz, sizeof(void *));
        }
    }
    np->children[np->num_children++] = cp;
}

static void
alloc_and_append_node(ast_node_t * np, ast_node_type_t typ)
{
    ast_node_t * child_node = zone_alloc(&zone, sizeof(*child_node));
    ast_node_init_type_cap(child_node, typ, 0);
    ast_node_append_child(np, child_node);
}

// TODO: 'void *' is allowed even though 'void' is not
//       void is allowed as return type
static ast_node_t *
parse_type()
{
    tokenizer_state_t saved_tok_state = tok_state;

    bool seen_const = false;
    bool seen_const_ptr = false;

    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type(np, AST_TYPE);
    np->line_num = saved_tok_state.line_num;

    token_t t = get_token();
    // TODO: do these belong here?
    if (t.typ == TOK_AUTO       ||
        t.typ == TOK_REGISTER   ||
        t.typ == TOK_STATIC     ||
        t.typ == TOK_EXTERN) {
        alloc_and_append_node(np, t.typ);
        t = get_token();
    }
    if (t.typ == TOK_CONST      ||
        t.typ == TOK_VOLATILE   ||
        t.typ == TOK_RESTRICT) {
        seen_const = true;
        alloc_and_append_node(np, t.typ);
        t = get_token();
    }
    //  TODO: used-defined types
    if (t.typ == TOK_SIGNED ||
        t.typ == TOK_UNSIGNED) {
        alloc_and_append_node(np, t.typ);
        t = get_token();
    }
    if (t.typ == TOK_INT   ||
        t.typ == TOK_CHAR  ||
        t.typ == TOK_LONG  ||
        t.typ == TOK_SHORT ||
        t.typ == TOK_FLOAT ||
        t.typ == TOK_DOUBLE) {
        alloc_and_append_node(np, t.typ);
        t = peek_token(0);
    } else {
        goto no_match;
    }
    if (t.typ == TOK_CONST      ||
        t.typ == TOK_VOLATILE   ||
        t.typ == TOK_RESTRICT) {
        if (seen_const) {
            // TODO: produce error
            goto no_match;
        }
        alloc_and_append_node(np, t.typ);
        t = peek_token(0);
    }
    if (t.typ == '*') {
        while (t.typ == '*') {
            get_token();
            alloc_and_append_node(np, '*');
            t = peek_token(0);
        }
        if (t.typ == TOK_CONST      ||
            t.typ == TOK_VOLATILE   ||
            t.typ == TOK_RESTRICT) {
            if (seen_const_ptr) {
                // TODO: produce error
                goto no_match;
            }
            seen_const_ptr = true;
            alloc_and_append_node(np, t.typ);
            get_token();
        }
    }
    return np;

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_ident()
{
    token_t t = get_token();
    if (t.typ == TOK_IDENT) {
        ast_node_t * np = zone_alloc(&zone, sizeof(*np));
        ast_node_init_type_cap(np, t.typ, 0);
        np->s = t.u.s;
        np->line_num = tok_state.line_num;
        return np;
    }
    return NULL;
}

// TODO: zone checkpoint and restore to quickly discard unneeded allocations?
static ast_node_t *
parse_var_decl()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * type_node,
               * ident_node;

    if (get_token().typ != TOK_EXTERN)  { goto no_match; }
    if (!(type_node = parse_type()))    { goto no_match; }
    if (!(ident_node = parse_ident()))  { goto no_match; }
    if (get_token().typ != ';')         { goto no_match; }

    // add children
    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type_cap(np, AST_VAR_DECL, 2);
    ast_node_append_child(np, type_node);
    ast_node_append_child(np, ident_node);
    np->line_num = saved_tok_state.line_num;
    return np;

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_var_def()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * type_node,
               * ident_node;

    if (!(type_node = parse_type()))    { goto no_match; }
    if (!(ident_node = parse_ident()))  { goto no_match; }
    // TODO: optional assignment
    if (get_token().typ != ';')         { goto no_match; }

    // add children
    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type_cap(np, AST_VAR_DEF, 3);
    ast_node_append_child(np, type_node);
    ast_node_append_child(np, ident_node);
    // TODO: assignment
    np->line_num = saved_tok_state.line_num;
    return np;

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_param_list()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * type_node,
               * ident_node;

    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type(np, AST_PARAM_LIST);
    np->line_num = saved_tok_state.line_num;

    // zero parameters
    if (peek_token(0).typ == ')') {
        return np;
    }

    bool first = true;
    while (1) {

        if (!first && get_token().typ != ',')           { goto no_match; }
        first = false;

        if (!(type_node = parse_type()))                { goto no_match; }
        if (!(ident_node = parse_ident()))              { goto no_match; }
        ast_node_append_child(np, type_node);
        ast_node_append_child(np, ident_node);

        // n parameters
        if (peek_token(0).typ == ')') {
            return np;
        }
    }

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_func_decl()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * type_node,
               * ident_node,
               * param_list_node;

    if (!(type_node = parse_type()))                { goto no_match; }
    if (!(ident_node = parse_ident()))              { goto no_match; }
    if (get_token().typ != '(')                     { goto no_match; }
    if (!(param_list_node = parse_param_list()))    { goto no_match; }
    if (get_token().typ != ')')                     { goto no_match; }
    if (get_token().typ != ';')                     { goto no_match; }

    // add children
    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type_cap(np, AST_FUNC_DECL, 3);
    ast_node_append_child(np, type_node);
    ast_node_append_child(np, ident_node);
    ast_node_append_child(np, param_list_node);
    np->line_num = saved_tok_state.line_num;
    return np;

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_expr()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type(np, AST_EXPR);
    np->line_num = saved_tok_state.line_num;

    // () [] -> .                           left to right
    // ! ~ ++ -- + - * & (type) sizeof      right to left
    // * / %                                left to right
    // + -                                  left to right
    // << >>                                left to right
    // < <= > >=                            left to right
    // == !=                                left to right
    // &                                    left to right
    // ^                                    left to right
    // |                                    left to right
    // &&                                   left to right
    // ||                                   left to right
    // ?:                                   right to left
    // = += -= *= /= %= &= ^= |= <<= >>=    right to left
    // ,                                    left to right

    //  j++;
    //  a = 3;
    //  b = 2*(a = 3);
    //  b *= 2;
    //  (x, y)
    // terms:
    //  literals
    //  variables
    //  function calls
    //  ()s
    // operators:
    //  binary: + - * / % && || & | 
    //  binary (lvalue required): += -= *= /= %= &= |=
    //  unary: ! ~ - *
    //  unary (lvalue required): & 
    //  postfix (lvalue required): ++ --
    //  struct: . ->
    // other:
    //  casts
    //  sizeof()

    while (1) {
        if (peek_token(0).typ == ';') {
            get_token();
            return np;
        }
        token_t t = get_token();
        if (t.typ == '(') {
            // either a cast or parenthetical expression
            // try cast first
            ast_node_t * cast_node = parse_type();
            if (cast_node) {
                t = get_token();
                if (t.typ != ')') { goto no_match; }
                cast_node->typ = AST_CAST;
                ast_node_append_child(np, cast_node);
            }
            // must be a parenthetical expression
            parse_expr();
            if (t.typ == ')') {
            }
        }
        //switch (t.typ) {
        //    case '!': break;
        //    case '~': break;
        //    case '-': break;
        //    case '*': break;
        //    default: ;
        //}
    }

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_stmt()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type(np, AST_STMT);
    np->line_num = saved_tok_state.line_num;

    token_t t = peek_token(0);

    // compound statement
    if (t.typ == '{') {
        get_token();
        ast_node_t * stmt_node;
        while (1) {
            if (peek_token(0).typ == '}') {
                get_token();
                return np;
            }
            if (!(stmt_node = parse_stmt()))    { goto no_match; }
            ast_node_append_child(np, stmt_node);
        }
    }

    // control statement
    switch (t.typ) {
        case TOK_IF:
            assert(0);
        case TOK_FOR:
            assert(0);
        case TOK_WHILE:
            assert(0);
        case TOK_DO:
            assert(0);
        case TOK_SWITCH:
            assert(0);
        case TOK_GOTO:
            assert(0);
        case TOK_RETURN:
        {
            get_token();
            ast_node_t * expr_node = parse_expr();
            if (!expr_node) { goto no_match; }
            ast_node_append_child(np, expr_node);
            return np;
        }
        default: ;
    }
    //if (t.typ == TOK_IF     ||
    //    t.typ == TOK_FOR    ||
    //    t.typ == TOK_WHILE  ||
    //    t.typ == TOK_DO     ||
    //    t.typ == TOK_SWITCH ||
    //    t.typ == TOK_GOTO   ||
    //    t.typ == TOK_RETURN) {
    //    get_token();
    //    if (t.typ == TOK_RETURN) {
    //    }
    //}

    // expression statement
    ast_node_t * expr_node = parse_expr();
    if (!expr_node) { goto no_match; }
    ast_node_append_child(np, expr_node);
    return np;

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_func_body()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type(np, AST_PARAM_LIST);
    np->line_num = saved_tok_state.line_num;

    while (1) {
        if (peek_token(0).typ == '}') {
            return np;
        }
        ast_node_t * stmt_node;
        //if ((stmt_node = parse_var_def())) {
        //    ast_node_append_child(np, stmt_node);
        //    continue;
        //}
        if ((stmt_node = parse_stmt())) {
            ast_node_append_child(np, stmt_node);
            continue;
        }
        break;
    }

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_func_def()
{
    tokenizer_state_t saved_tok_state = tok_state;

    ast_node_t * type_node,
               * ident_node,
               * param_list_node,
               * func_body_node;

    if (!(type_node = parse_type()))                { goto no_match; }
    if (!(ident_node = parse_ident()))              { goto no_match; }
    if (get_token().typ != '(')                     { goto no_match; }
    if (!(param_list_node = parse_param_list()))    { goto no_match; }
    if (get_token().typ != ')')                     { goto no_match; }
    if (get_token().typ != '{')                     { goto no_match; }
    if (!(func_body_node = parse_func_body()))      { goto no_match; }
    if (get_token().typ != '}')                     { goto no_match; }

    // add children
    ast_node_t * np = zone_alloc(&zone, sizeof(*np));
    ast_node_init_type_cap(np, AST_FUNC_DEF, 4);
    ast_node_append_child(np, type_node);
    ast_node_append_child(np, ident_node);
    ast_node_append_child(np, param_list_node);
    ast_node_append_child(np, func_body_node);
    np->line_num = saved_tok_state.line_num;
    return np;

no_match:
    tok_state = saved_tok_state;
    return NULL;
}

static ast_node_t *
parse_struct_decl()
{
no_match:
    return NULL;
}

static ast_node_t *
parse_struct_def()
{
no_match:
    return NULL;
}

static ast_node_t *
parse_union_decl()
{
no_match:
    return NULL;
}

static ast_node_t *
parse_union_def()
{
no_match:
    return NULL;
}

static ast_node_t *
parse_enum_decl()
{
no_match:
    return NULL;
}

static ast_node_t *
parse_enum_def()
{
no_match:
    return NULL;
}

static ast_node_t *
parse_typedef()
{
no_match:
    return NULL;
}

static ast_node_t *
parse_tu()
{
    ast_node_t * ast = zone_alloc(&zone, sizeof(*ast));
    ast_node_init_type(ast, AST_TU);
    while (1) {
        if (peek_token(0).typ == TOK_EOF) {
            return ast;
        }
        ast_node_t * np;

#define TRY(PARSE_FUNC) \
        if ((np = PARSE_FUNC())) { \
            ast_node_append_child(ast, np); \
            continue; \
        } else { \
        }

        TRY(parse_var_decl);
        TRY(parse_var_def);
        TRY(parse_func_decl);
        TRY(parse_func_def);
        TRY(parse_struct_decl);
        TRY(parse_struct_def);
        TRY(parse_union_decl);
        TRY(parse_union_def);
        TRY(parse_enum_decl);
        TRY(parse_enum_def);
        TRY(parse_typedef);
        break;
    }
    return NULL;
}

typedef struct node_t node_t;

struct node_t {
    char c;
    node_t * left,
           * right;
};

static node_t *
f(const char * s)
{
    node_t * np = malloc(sizeof(*np));
    node_t * np2, * np3;
    *np = (node_t) { .c = *s };

    if (*s == '&') {
        s++;
        // if *(s+1) == '\0', error
        np2 = malloc(sizeof(*np2));
        *np2 = (node_t) { .c = *s };
        np->right = np2;
    } else {
        np2 = np;
    }
    s++;

    //if (!*s)
    //    return np;

    if (*s == '^') {
        np3 = malloc(sizeof(*np3));
        *np3 = *np2;
        *np2 = (node_t) { .c = *s, .right = np3 };
        s++;
    } else {
        np3 = np2;
    }

    //if (!*s)
    //    return np;

    if (*s == '+' ||
        *s == '-' ||
        *s == '*' ||
        *s == '/') {
        node_t * np4 = malloc(sizeof(*np4));
        *np4 = *np3;
        np3->c = *s++;
        np3->left = np4;
        np3->right = f(s);
    }

    return np;
}

static int
is_op(char c)
{
    if (c == '+' ||
        c == '-' ||
        c == '*' ||
        c == '/') {
        return true;
    }
    return false;
}

static int
prec(char c)
{
    switch (c) {
        case '^':
        case '&': return 3;
        case '*': return 2;
        case '+':
        case '-': return 1;
        default: assert(0);
    }
}

static node_t *
fixup(node_t * np)
{
    if (!np || !np->right)
        return np;
    if (!is_op(np->right->c))
        return np;
    np->right = fixup(np->right);
    if (prec(np->right->c) <= prec(np->c)) {
        node_t * tmp = np->right;
        np->right = np->right->left;
        tmp->left = np;
        np = tmp;
        np->left = fixup(np->left);
    }
    return np;
}

static void
print_node(node_t * np, int depth)
{
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    if (!np) {
        printf("(null)\n");
        return;
    }
    printf("%c\n", np->c);
    if (np->left || np->right) {
        print_node(np->left, depth+1);
        print_node(np->right, depth+1);
    }
}

int main(int argc, char * argv[])
{
#if 0
    (void) argc;
    (void) argv;
    zone = zone_init(1<<20);
    ast_node_t * ast = parse_tu();
    if (!ast)
        return EXIT_FAILURE;
    ast_print(ast, stdout);
    zone_deinit(&zone);
    return EXIT_SUCCESS;
#endif
    node_t * np = f("a+&b*c^-d");
    //node_t * np = f("a*b-c+d*e+f");
    print_node(np, 0);
    np = fixup(np);
    print_node(np, 0);
    return 0;
}
