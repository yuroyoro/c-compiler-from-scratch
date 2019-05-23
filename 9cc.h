#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

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
  TK_ELSE,      // else
  TK_WHILE,     // while
  TK_FOR,       // for
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
  ND_RETURN,    // return
  ND_IF,        // if
  ND_WHILE,     // while
  ND_FOR,       // for
  ND_BLOCK,     // block
  ND_CALL,      // function call
  ND_STMT,      // statement
  ND_EXPR,      // expr
  ND_FUNC,      // function definition
  ND_VAR_DEF,   // variable definition
  ND_VAR_REF,   // variable reference
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LE,        // <=
};

typedef struct Scope {
  Map *lvars ;    // local variables
  int var_cnt ;   // local variables count
  int stacksize ; // stack size

  // TODO: scope chain
} Scope;

typedef struct Node {
  int    op;        // operator or ND_NUM

  /// lhs `binop` rhs
  struct Node *lhs;  // left hand side
  struct Node *rhs;  // right hand side

  // "return" expr
  struct Node *expr; // expression

  // "if" ( cond ) then "else" els
  // "while" ( cond ) body
  // "for" ( init; cond; inc ) bdoy
  struct Node *cond;
  struct Node *then;
  struct Node *els;
  struct Node *body;
  struct Node *init;
  struct Node *inc;

  Vector *stmts;
  Vector *args;

  Scope *scope;

  int    val;       // number value if ty == ND_NUM
  char   *name;     // identifier name
  int    offset;    // offset from stack pointer
} Node;

extern Vector *code;

void program() ;
void dump_node(char *msg, Node *node) ;
char *node_string(int ty) ;

// code_gen.c
void generate() ;

// main.c
extern bool debug ;
void error(char *fmt, ...) ;

// test
void runtest() ;
