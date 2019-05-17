#include "9cc.h"

const char *NODE_STRING[] = {
  STRING(ND_NUM),
  STRING(ND_IDENT),
  STRING(ND_RETURN),
  STRING(ND_IF),
  STRING(ND_EQ),
  STRING(ND_NE),
  STRING(ND_LE),
};

char *node_string(int ty) {
  char *str;
  if (ty < 256) {
    str = malloc(sizeof(char) * 2);
    sprintf(str, "%c", ty);
    return str;
  }

  str = strndup(NODE_STRING[ty - 256], strlen(NODE_STRING[ty - 256]));
  return str;
}

void dump_node(Node *n) {
  printf("# node %-10s : ty = %d, val = %d, name = [%s]\n", node_string(n->ty), n->ty, n->val, n->name);
}

// Abstract syntax tree

Vector *tokens;
int pos = 0; // current token position

// variables
Map *vars ;
int var_cnt = 0; // variable counter

static Node *new_node(int ty) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  return node;
}

static Node *new_node_bin_op(int ty, Node *lhs, Node *rhs) {
  Node *node = new_node(ty);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_node_ident(char *name) {
  Node *node = new_node(ND_IDENT);
  node->name = name;

  // update variables map and counter
  void *offset = map_get(vars, name);
  if (offset == NULL) {
    map_puti(vars, name, var_cnt++);
  }

  return node;
}

static Node *new_node_expr(int ty, Node *expr) {
  Node *node = new_node(ty);
  node->expr = expr;
  return node;
}

static Node *new_node_cond_expr(int ty, Node *cond, Node *expr) {
  Node *node = new_node(ty);
  node->cond = cond;
  node->expr = expr;
  return node;
}
/*
  program    = { stmt }
  stmt       = expr ";"
             | "return" expr ";"
             | "if" "(" expr ")" stmt [ "else" stmt ]
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

void trace_parse(char *name) {
  if (debug) {
    Token *t = current_token();
    printf("# parse : %-10s : token %-10s : ty = %d, val = %d, input = [%s]\n", name, token_string(t->ty), t->ty, t->val, t->input);
  }
}

static int consume(int ty) {
  Token *t = current_token();
  if (t->ty != ty) {
    return 0;
  }

  pos++;
  return 1;
}

static void expect(char c) {
  if (!consume(c)) {
    Token *t = current_token();
    error("expected '%c' : %s", t->input);
  }
}

void program() {
  trace_parse("program");

  int i = 0;
  vars = new_map();

  while(current_token()->ty != TK_EOF) {
    code[i++] = stmt();
  }
  code[i] = NULL;
}

Node *stmt() {
  trace_parse("stmt");

  Node *node;

  if (consume(TK_RETURN)) {
    node = new_node_expr(ND_RETURN, expr());
    Token *t = current_token();
    if (!consume(';') && t->ty != TK_EOF) {
      error("expected ';' : %s", t->input);
    }
  } else if (consume(TK_IF)) {
    expect('(');
    Node *cond = expr();
    expect(')');
    Node *expr = stmt();

    node = new_node_cond_expr(ND_IF, cond, expr);
  } else {
    node = expr();

    Token *t = current_token();
    if (!consume(';') && t->ty != TK_EOF) {
      error("expected ';' : %s", t->input);
    }
  }

  return node;
}

Node *expr() {
  trace_parse("expr");

  return assign();
}

Node *assign() {
  trace_parse("assign");

  Node *node = equality();
  if (consume('=')) {
    node = new_node_bin_op('=', node, assign());
  }

  return node;
}

Node *equality() {
  trace_parse("equality");

  Node *node = relational();

  for (;;) {
    if (consume(TK_EQ)){
      node = new_node_bin_op(ND_EQ, node, relational());
    } else if (consume(TK_NE)) {
      node = new_node_bin_op(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

static Node *relational() {
  trace_parse("relational");

  Node *node = add();

  for (;;) {
    if (consume(TK_LE)){
      node = new_node_bin_op(ND_LE, node, add());
    } else if (consume('<')) {
      node = new_node_bin_op('<', node, add());
    } else if (consume(TK_GE)) {
      node = new_node_bin_op(ND_LE, add(), node);
    } else if (consume('>')) {
      node = new_node_bin_op('<', add(), node);
    } else {
      return node;
    }
  }
}

static Node *add() {
  trace_parse("add");

  Node *node = mul();

  for (;;) {
    if (consume('+')) {
      node = new_node_bin_op('+', node, mul());
    } else if (consume('-')) {
      node = new_node_bin_op('-', node, mul());
    } else {
      return node;
    }
  }
}

static Node *mul() {
  trace_parse("mul");

  Node *node = unary();

  for (;;) {
    if (consume('*')) {
      node = new_node_bin_op('*', node, unary());
    } else if (consume('/')) {
      node = new_node_bin_op('/', node, unary());
    } else {
      return node;
    }
  }
}

static Node *term() {
  trace_parse("term");

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
    return new_node_ident(next_token()->name);
  }

  error("invalid token: %s", t->input);
  exit(1);
}

static Node *unary() {
  if (consume('+')) {
    return term();
  }
  if (consume('-')) {
    return new_node_bin_op('-', new_node_num(0), term());
  }

  return term();
}
