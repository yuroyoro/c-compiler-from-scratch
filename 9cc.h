#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// container.c
typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector() ;
void vec_push(Vector *vec, void *elem) ;

// token.c
enum {
  TK_NUM = 256, // number
  TK_EQ, // ==
  TK_NE, // !=
  TK_LE, // <=
  TK_GE, // >=

  TK_EOF,       // terminator
};

typedef struct {
  int ty;       // type of Tokens
  int val;      // number value if ty == TK_NUM
  char *input;  // token string (for error messsage)
} Token;

Vector *tokenize(char *p) ;

// parse.c
extern Vector *tokens;
extern int pos ; // current token position

enum {
  ND_NUM = 256, // number
  ND_EQ, // ==
  ND_NE, // !=
  ND_LE, // <=
};

typedef struct Node {
  int    ty;        // operator or ND_NUM
  struct Node *lhs; // left hand side
  struct Node *rhs; // right hand side
  int    val;       // number value if ty == ND_NUM
} Node;

Node *equality() ; // TODO: rename

// code_gen.c
void gen(Node *node) ;

// main.c
void error(char *fmt, ...) ;

// test
void runtest() ;
