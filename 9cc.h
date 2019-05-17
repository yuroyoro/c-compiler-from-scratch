#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>

// container.c
typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector() ;
void vec_push(Vector *vec, void *elem) ;

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

Map *new_map() ;
void map_put(Map *map, char *key, void *val) ;
void *map_get(Map *map, char *key) ;
void map_puti(Map *map, char *key, int i) ;
int  map_geti(Map *map, char *key) ;

// token.c

#define STRING(var) #var

enum {
  TK_NUM = 256, // number
  TK_IDENT,     // identifier
  TK_RETURN,    // return
  TK_IF,        // if
  TK_EQ,        // ==
  TK_NE,        // !=
  TK_LE,        // <=
  TK_GE,        // >=

  TK_EOF,       // terminator
};

typedef struct {
  int  ty;      // type of Tokens
  int  val;     // number value if ty == TK_NUM
  char *name;   // identifier name
  char *input;  // token string (for error messsage)
} Token;

Vector *tokenize(char *p) ;
void dump_token(int i, Token *token) ;
char *token_string(int ty) ;

// parse.c
extern Vector *tokens;
extern int pos ; // current token position

enum {
  ND_NUM = 256, // number
  ND_IDENT,     // identifier
  ND_RETURN,    // return
  ND_IF,        // if
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LE,        // <=
};

typedef struct Node {
  int    ty;        // operator or ND_NUM

  struct Node *lhs;  // left hand side
  struct Node *rhs;  // right hand side
  struct Node *expr; // expression
  struct Node *cond; // if condition

  int    val;       // number value if ty == ND_NUM
  char   *name;     // identifier name
} Node;

#define CODE_SIZE 100
extern Node *code[CODE_SIZE];

extern Map *vars ; // variables
extern int var_cnt ;

void program() ;
void dump_node(Node *node) ;
char *node_string(int ty) ;

// code_gen.c
void generate() ;

// main.c
extern bool debug ;
void error(char *fmt, ...) ;

// test
void runtest() ;
