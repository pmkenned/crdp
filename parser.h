#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

typedef struct ast_node_t ast_node_t;

void ast_print(ast_node_t * ast, FILE * fp);
ast_node_t * parse_tu();

#endif /* PARSER_H */
