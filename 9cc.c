#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tokens
enum {
  TK_NUM = 256, // number
  TK_EOF,       // terminator
};

typedef struct {
  int ty;       // type of Tokens
  int val;      // number value if ty == TK_NUM
  char *input;  // token string (for error messsage)
} Token;

// put result of tgkenized tokens into bellow.
// we assume that do not input over 100 tokens
Token tokens[100];

// error reporting fuction
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// split string that p pointed to token, and store them to tokens
void tokenize(char *p) {
  int i = 0;
  while(*p) {
    // skip white spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' ) {
      tokens[i].ty = *p;
      tokens[i].input = p;
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p, &p, 10);
      i++;
      continue;
    }

    error("could not tokenize: %s", p);
    exit(1);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

// Abstract syntax tree

int pos = 0; // current token position

enum {
  ND_NUM = 256, // number
};

typedef struct Node {
  int    ty;        // operator or ND_NUM
  struct Node *lhs; // left hand side
  struct Node *rhs; // right hand side
  int    val;       // number value if ty == ND_NUM
} Node;

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

// paser functions
Node *add() ;
Node *mul() ;
Node *term() ;

int consume(int ty) {
  if (tokens[pos].ty != ty) {
    return 0;
  }

  pos++;
  return 1;
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume('+')) {
      node = new_node('+', node, mul());
    } else if (consume('-')) {
      node = new_node('-', node, mul());
    } else {
      return node;
    }
  }
}

Node *mul() {
  Node *node = term();

  for (;;) {
    if (consume('*')) {
      node = new_node('*', node, term());
    } else if (consume('/')) {
      node = new_node('/', node, term());
    } else {
      return node;
    }
  }
}

Node *term() {
  // if next token is '(', "(" add ")" is expected
  if (consume('(')) {
    Node *node = add();
    if (!consume(')')) {
      error("expected ')' : %s", tokens[pos].input);
    }
    return node;
  }
  // otherwise it should be a number
  if (tokens[pos].ty == TK_NUM) {
    return new_node_num(tokens[pos++].val);
  }

  error("invalid token: %s", tokens[pos].input);
  exit(1);
}

// code generator
void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf(" push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);
  printf(  "pop rdi\n");
  printf(  "pop rax\n");

  switch (node->ty) {
    case '+':
      printf("  add rax, rdi\n");
      break;
    case '-':
      printf("  sub rax, rdi\n");
      break;
    case '*':
      printf("  mul rdi\n");
      break;
    case '/':
      printf("  mov rdx, 0\n");
      printf("  div rdi\n");
      break;
  }

  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "invalid argument count\n");
    return 1;
  }

  // tokenize input
  tokenize(argv[1]);
  Node *node = add();

  // print assembler headers
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // generate assembler codes by traversing AST
  gen(node);

  // stack top value should be the result of whole program expression.
  // load it to rax to return code
  printf("  pop rax\n");
  printf("  ret\n");

  return 0;
}
