#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Tokens
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

Token *new_token(int ty, char *input) {
  Token *token = malloc(sizeof(Token));
  token->ty = ty;
  token->input = input;
  return token;
}

// Vector
typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector() {
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void *) * 16);
  vec->capacity = 16;
  vec->len = 0;
  return vec;
}

void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec-> capacity);
  }

  vec->data[vec->len++] = elem;
}

// error reporting fuction
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static struct {
  char *name;
  int ty;
} symbols[] = {
    {"!=", TK_NE}, {"<=", TK_LE},
    {"==", TK_EQ}, {">=", TK_GE},
    {NULL, 0},
};

bool startswith(char *s1, char *s2) {
  return !strncmp(s1, s2, strlen(s2));
}

// split string that p pointed to token, and store them to Vector
Vector *tokenize(char *p) {
  Vector *vec = new_vector();

  while(*p) {
    // skip white spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    bool found = false;
    for (int i = 0; symbols[i].name; i++) {
      char *name = symbols[i].name;
      if (!startswith(p, name)) {
        continue;
      }

      Token *token = new_token(symbols[i].ty, name);
      vec_push(vec, (void *)token);

      p += strlen(name);
      found = true;
      break;
    }
    if (found) {
      continue;
    }

    if (strchr("+-*/()<>", *p)) {
      Token *token = new_token(*p, p);
      vec_push(vec, (void *)token);
      p++;
      continue;
    }

    if (isdigit(*p)) {
      Token *token = new_token(TK_NUM, p);
      vec_push(vec, (void *)token);
      token->val = strtol(p, &p, 10);
      continue;
    }

    error("could not tokenize: %s", p);
    exit(1);
  }

  Token *token = new_token(TK_EOF, p);
  vec_push(vec, (void *)token);

  return vec;
}

// Abstract syntax tree

Vector *tokens;
int pos = 0; // current token position

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

/*
  equality: relational
  equality: equality "==" relational
  equality: equality "!=" relational

  relational: add
  relational: relational "<"  add
  relational: relational "<=" add
  relational: relational ">"  add
  relational: relational ">=" add

  add: mul
  add: add "+" mul
  add: add "-" mul

  mul: unary
  mul: mul "*" unary
  mul: mul "/" unary

  unary: term
  unary: "+" term
  unary: "-" term

  term: num
  term: "(" equality ")"
*/

// paser functions
// TODO: split header file
Node *equality() ;
Node *relational();
Node *add() ;
Node *mul() ;
Node *term() ;
Node *unary() ;

Token *current_token() {
  return (Token *)tokens->data[pos];
}

Token *next_token() {
  return (Token *)tokens->data[pos++];
}

int consume(int ty) {
  Token *t = current_token();
  if (t->ty != ty) {
    return 0;
  }

  pos++;
  return 1;
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume(TK_EQ)){
      node = new_node(ND_EQ, node, relational());
    } else if (consume(TK_NE)) {
      node = new_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume(TK_LE)){
      node = new_node(ND_LE, node, add());
    } else if (consume('<')) {
      node = new_node('<', node, add());
    } else if (consume(TK_GE)) {
      node = new_node(ND_LE, add(), node);
    } else if (consume('>')) {
      node = new_node('<', add(), node);
    } else {
      return node;
    }
  }
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
  Node *node = unary();

  for (;;) {
    if (consume('*')) {
      node = new_node('*', node, unary());
    } else if (consume('/')) {
      node = new_node('/', node, unary());
    } else {
      return node;
    }
  }
}

Node *term() {
  Token *t = current_token();

  // if next token is '(', "(" add ")" is expected
  if (consume('(')) {
    Node *node = equality();
    if (!consume(')')) {
      error("expected ')' : %s", t->input);
    }
    return node;
  }
  // otherwise it should be a number
  if (t->ty == TK_NUM) {
    return new_node_num(next_token()->val);
  }

  error("invalid token: %s", t->input);
  exit(1);
}

Node *unary() {
  if (consume('+')) {
    return term();
  }
  if (consume('-')) {
    return new_node('-', new_node_num(0), term());
  }

  return term();
}

// code generator
void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  # node num %d\n", node->val);
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  # node %d\n", node->ty);
  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
    case '<':
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
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
      printf("  mov rdx, 0\n"); // TODO: signed int
      printf("  div rdi\n");
      break;
  }

  printf("  push rax\n");
}

// unit test for Vector
void expect(int line, int expected, int actual) {
  if (expected == actual) {
    return;
  }

  fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
  exit(1);
}

void runtest() {
  Vector *vec = new_vector();
  expect(__LINE__, 0, vec->len);

  for (int i = 0; i < 100; i++) {
    vec_push(vec, (void *)(long)i);
  }

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, (long)vec->data[0]);
  expect(__LINE__, 50, (long)vec->data[50]);
  expect(__LINE__, 99, (long)vec->data[99]);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "invalid argument count\n");
    return 1;
  }

  if (!strncmp(argv[1], "-test", 5)) {
    runtest();
    return 0;
  }

  // tokenize input
  tokens = tokenize(argv[1]);

  for(int i = 0; i < tokens->len; i++) {
    Token *t = (Token *)tokens->data[i];
    printf("# token %2d : ty = %d, val = %d, input = %s\n", i, t->ty, t->val, t->input);
  }

  Node *node = equality();

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
