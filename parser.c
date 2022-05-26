#include "parser.h"
#include "tokenizer.h"
#include "arena.h"
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>

#define AST_ENUMS   \
    X(TU)           \
    X(TYPE)         \
    X(CAST)         \
    X(VAR_DECL)     \
    X(VAR_DEF)      \
    X(FUNC_DECL)    \
    X(FUNC_DEF)     \
    X(STRUCT_DECL)  \
    X(STRUCT_DEF)   \
    X(UNION_DECL)   \
    X(UNION_DEF)    \
    X(ENUM_DECL)    \
    X(ENUM_DEF)     \
    X(TYPEDEF_DEF)  \
    X(PARAM_LIST)   \
    X(STMT)         \
    X(STMT_LIST)    \
    X(EXPR)

#define X(A) AST_ ## A,
typedef enum {
    AST_EOF = 256,
    TOK_ENUMS
    AST_ENUMS
} ast_node_type_t;
#undef X

struct ast_node_t {
    ast_node_type_t typ;
    char * s;
    int line_num;
    ast_node_t ** children;
    size_t num_children;
    size_t children_cap;
};

#define X(A) [AST_ ## A] = #A,
static const char * enum_strs[] = {
    X(EOF)
    TOK_ENUMS
    AST_ENUMS
};
#undef X

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

void
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
        np->children = arena_alloc(&arena, sizeof(*np->children)*cap);
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
            np->children = arena_alloc_align(&arena, sizeof(*np->children)*cap, 8);
        } else {
            size_t old_sz = sizeof(*np->children)*np->children_cap;
            size_t new_sz = sizeof(*np->children)*(np->children_cap *= 2);
            np->children = arena_realloc_align(&arena, np->children, old_sz, new_sz, sizeof(void *));
        }
    }
    np->children[np->num_children++] = cp;
}

static void
alloc_and_append_node(ast_node_t * np, ast_node_type_t typ)
{
    ast_node_t * child_node = arena_alloc(&arena, sizeof(*child_node));
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

    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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
        ast_node_t * np = arena_alloc(&arena, sizeof(*np));
        ast_node_init_type_cap(np, t.typ, 0);
        np->s = t.u.s;
        np->line_num = tok_state.line_num;
        return np;
    }
    return NULL;
}

// TODO: arena checkpoint and restore to quickly discard unneeded allocations?
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
    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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
    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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

    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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
    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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

    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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
        token_t t = peek_token(0);
        if (t.typ == ';' ||
            t.typ == ')') {
            return np;
        }
        t = get_token();
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
            ast_node_t * paren_expr_node = parse_expr();
            t = get_token();
            if (t.typ == ')') {
                ast_node_append_child(np, paren_expr_node);
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

    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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
            if (!expr_node)             { goto no_match; }
            if (get_token().typ != ';') { goto no_match; }
            ast_node_append_child(np, expr_node);
            return np;
        }
        default: ;
    }

    // expression statement
    ast_node_t * expr_node = parse_expr();
    if (!expr_node)             { goto no_match; }
    if (get_token().typ != ';') { goto no_match; }
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

    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
    ast_node_init_type(np, AST_STMT_LIST);
    np->line_num = saved_tok_state.line_num;

    while (1) {
        if (peek_token(0).typ == '}') {
            return np;
        }
        ast_node_t * stmt_node;
        if ((stmt_node = parse_var_def())) {
            ast_node_append_child(np, stmt_node);
            continue;
        }
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
    ast_node_t * np = arena_alloc(&arena, sizeof(*np));
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

ast_node_t *
parse_tu()
{
    ast_node_t * ast = arena_alloc(&arena, sizeof(*ast));
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
