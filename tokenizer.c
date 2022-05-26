#include "tokenizer.h"
#include <stdio.h>
#include <ctype.h>

#define NELEMS(X) (sizeof(X)/sizeof(X[0]))
#define NELEMSU(X) (int)(sizeof(X)/sizeof(X[0]))

tokenizer_state_t tok_state = { 0, 1 };

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
    { .typ = TOK_INT,                   },
    { .typ = TOK_IDENT, .u.c_s = "x"    },
    { .typ = ';',                       },
    { .typ = TOK_RETURN,                },
    { .typ = TOK_LITERAL_INT, .u.i = 0  },
    { .typ = ';',                       },
    { .typ = '}',                       },
    { .typ = '\n',                      },

    { .typ = TOK_EOF,                   },
};

token_t
peek_token(int n)
{
    token_t t;
    int peek_idx = tok_state.token_idx;
    for (int i = 0; i <= n; i++) {
        do {
            t = token_list[peek_idx++];
            if (t.typ == TOK_EOF) {
                return t;
            }
        } while (t.typ == '\n');
    }
    return t;
}

#define X(A) [TOK_ ## A] = #A,
static const char * tok_enum_strs[] = {
    TOK_ENUMS
};
#undef X

token_t
get_token()
{
    token_t t;
    if (tok_state.token_idx >= NELEMSU(token_list)) {
        return (token_t) { .typ = TOK_EOF };
    }
    do {
        t = token_list[tok_state.token_idx++];
#if 0
        const char * enum_str = tok_enum_strs[t.typ];
        if (enum_str == NULL) {
            if (isprint(t.typ))
                fprintf(stderr, " '%c'", t.typ);
            else
                fprintf(stderr, " %d", t.typ);
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
    const char * enum_str = tok_enum_strs[c];
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
