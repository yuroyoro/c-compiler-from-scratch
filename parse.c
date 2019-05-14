#include "9cc.h"

// Abstract syntax tree

Vector *tokens;
int pos = 0; // current token position

static Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

static Node *new_node_ident(char name) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  return node;
}
/*
  program    = { stmt }
  stmt       = [ "return" ] expr (";" | EOF)
  expr       = assign
  assign     = equality [ "=" assign ]
  equality   = relational { ( "==" | "!=" ) relational }
  relational = add { ( "<" | "<=" | ">" | ">=" ) add }
  add        = mul { ( "+" | "-" ) mul }
  mul        = unary { ( "*" | "/" ) unary }
  unary      = [ "+" | "-" ] term
  term       = num | "(" expr ")" term: "(" equality ")"
*/

Node *code[100];

// paser functions
static Node *stmt() ;
static Node *expr() ;
static Node *assign() ;
static Node *equality();
static Node *relational();
static Node *add() ;
static Node *mul() ;
static Node *term() ;
static Node *unary() ;

static Token *current_token() {
  return (Token *)tokens->data[pos];
}

static Token *next_token() {
  return (Token *)tokens->data[pos++];
}

static int consume(int ty) {
  Token *t = current_token();
  if (t->ty != ty) {
    return 0;
  }

  pos++;
  return 1;
}

void program() {
  int i = 0;
  while(current_token()->ty != TK_EOF) {
    code[i++] = stmt();
  }
  code[i] = NULL;
}

Node *stmt() {
  Node *node;
  if (consume(TK_RETURN)) {
    node = malloc(sizeof(Node));
    node->ty = ND_RETURN;
    node->lhs = expr();
  } else {
    node = expr();
  }

  Token *t = current_token();
  if (!consume(';') && t->ty != TK_EOF) {
    error("expected ';' : %s", t->input);
  }

  return node;
}

Node *expr() {
  return assign();
}

Node *assign() {
  Node *node = equality();
  if (consume('=')) {
    node = new_node('=', node, assign());
  }

  return node;
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

static Node *relational() {
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

static Node *add() {
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

static Node *mul() {
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

static Node *term() {
  Token *t = current_token();

  // if next token is '(', "(" add ")" is expected
  if (consume('(')) {
    Node *node = equality();
    if (!consume(')')) {
      error("expected ')' : %s", t->input);
    }
    return node;
  }

  // number
  if (t->ty == TK_NUM) {
    return new_node_num(next_token()->val);
  }

  // identifier
  if (t->ty == TK_IDENT) {
    return new_node_ident(*next_token()->input);
  }

  error("invalid token: %s", t->input);
  exit(1);
}

static Node *unary() {
  if (consume('+')) {
    return term();
  }
  if (consume('-')) {
    return new_node('-', new_node_num(0), term());
  }

  return term();
}
