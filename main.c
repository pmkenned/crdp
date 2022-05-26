#include "arena.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#if 0
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
#endif

int main(int argc, char * argv[])
{
#if 1
    (void) argc;
    (void) argv;
    arena = arena_init(1<<20);
    ast_node_t * ast = parse_tu();
    if (!ast)
        return EXIT_FAILURE;
    ast_print(ast, stdout);
    arena_deinit(&arena);
    return EXIT_SUCCESS;
#else
    //node_t * np = f("a+&b*c^-d");
    node_t * np = f("a*b-c+d*e+f");
    print_node(np, 0);
    np = fixup(np);
    print_node(np, 0);
    return 0;
#endif
}
