#include "9cc.h"

const char *NODE_STRING[] = {
  STRING(ND_NUM),
  STRING(ND_RETURN),
  STRING(ND_IF),
  STRING(ND_WHILE),
  STRING(ND_FOR),
  STRING(ND_BLOCK),
  STRING(ND_CALL),
  STRING(ND_STMT),
  STRING(ND_EXPR),
  STRING(ND_FUNC),
  STRING(ND_VAR_DEF),
  STRING(ND_VAR_REF),
  STRING(ND_EQ),
  STRING(ND_NE),
  STRING(ND_LE),
  STRING(ND_DEREF),
  STRING(ND_ADDR),
};

char *node_string(int op) {
  char *str;
  if (op < 256) {
    str = malloc(sizeof(char) * 2);
    sprintf(str, "%c", op);
    return str;
  }

  str = strndup(NODE_STRING[op - 256], strlen(NODE_STRING[op - 256]));
  return str;
}

#define INT_TYPE "int"

// Abstract syntax tree

Vector *tokens;
int pos = 0; // current token position

// scope
Scope *scope ; // TODO: use stack for scope chain

static Scope *new_scope() {
  Scope *scope = malloc(sizeof(Scope));
  scope->lvars = new_map();
  scope->stacksize = 0;
  return scope;
}

static Type *new_type(int ty, Type *ptr) {
  Type *t = malloc(sizeof(Type));
  t->ty = ty;
  t->ptrof = ptr;

  return t;
}

static Node *new_node(int op) {
  Node *node = malloc(sizeof(Node));
  node->op = op;

  if (debug) {
    printf("#   create node %-10s : op = %d\n", node_string(node->op), node->op);
  }

  return node;
}

static Node *new_stmt(Node *body) {
  Node *node = new_node(ND_STMT);
  node->body = body;

  return node;
}

static Node *new_expr(Node *expr) {
  Node *node = new_node(ND_EXPR);
  Node *e = expr;
  while (e->op == ND_EXPR) {
    e = e->expr;
  }

  node->expr = e;

  return node;
}

static Node *new_node_var_def(Type *ty, char *name ) {
  Node *node = new_node(ND_VAR_DEF);

  node->name = name;
  node->ty   = ty;

  scope->stacksize += 8;
  scope->var_cnt++;
  node->offset = scope->stacksize;
  map_put(scope->lvars, name, node);

  return node;
};

static Node *new_deref(Node *expr) {
  Node *node = new_node(ND_DEREF);
  if (expr->op != ND_EXPR) {
    expr = new_expr(expr);
  }

  node->expr = expr;

  return node;
}

static Node *new_address_of(Node *expr) {
  Node *node = new_node(ND_ADDR);
  if (expr->op != ND_EXPR) {
    expr = new_expr(expr);
  }

  node->expr = expr;

  return node;
}

static Node *new_node_bin_op(int op, Node *lhs, Node *rhs) {
  Node *node = new_node(op);
  node->lhs = new_expr(lhs);
  node->rhs = new_expr(rhs);
  return node;
}

static Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_node_var_ref(char *name) {
  Node *node = new_node(ND_VAR_REF);
  node->name = name;

  if (debug) {
    printf("  # new_node_var_ref : name = %s\n", name);
  }

  // update variables map and counter
  Node *tynode = (Node *)map_get(scope->lvars, name);
  if (tynode == NULL) {
    error("undefined variable '%s' : pos = %d", name, pos);
  }

  node->offset = tynode->offset;
  node->ty     = tynode->ty;

  return node;
}

static Node *new_node_expr(int op, Node *expr) {
  Node *node = new_node(op);
  node->expr = expr;
  return node;
}

static Node *new_node_cond(int op, Node *cond) {
  Node *node = new_node(op);
  node->cond = cond;
  return node;
}

/*
  program    = func *
  func       = typedef "("  (typedef ",")* ")" block
  block      = "{" stmt* "}"
  stmt       = block
             | expr ";"
             | "return" expr ";"
             | "if" "(" expr ")" stmt [ "else" stmt ]
             | "while" "(" expr ")" stmt
             | "for" "(" expr? ";" expr? ";" expr? ")" stmt
             | typedef ";"
  typedef    = "int" "*"* ident
  expr       = assign
  assign     = equality [ "=" assign ]
  equality   = relational ("==" relational | "!=" relational)*
  relational = add ("<" add | "<=" add | ">" add | ">=" add)*
  add        = mul ("+" mul | "-" mul)*
  mul        = unary ("*" unary | "/" unary)*
  unary      = (("+" | "-")? term | ("*" | "&") mul
  term       = num | ident | call | (" expr ")"
  call       = ident "(" (expr ",")* ")"
  ident      = A-Za-z0-9_
*/

Vector *code ;

// paser functions
static Node *block() ;
static Node *function() ;
static Node *stmt() ;
static Node *expr() ;
static Node *assign() ;
static Node *equality();
static Node *relational();
static Node *add() ;
static Node *mul() ;
static Node *term() ;
static Node *unary() ;

static Type int_ty = {INT, NULL};

static Token *token_at(int offset) {
  assert(tokens->data[pos+offset] != NULL);
  return (Token *)tokens->data[pos+offset];
}

static Token *current_token() {
  return token_at(0);
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

  if (debug) {
    printf("# parse : consume %s : pos = %d\n", token_string(ty), pos);
  }

  pos++;
  return 1;
}

static Token *expect(int c) {
  Token *t = current_token();

  if (!consume(c)) {
    error("expected '%c' : %s : pos = %d", c, t->input, pos);
  }
  return t;
}

void program() {
  trace_parse("program");

  code = new_vector();

  while(current_token()->ty != TK_EOF) {
    vec_push(code, function());
  }
}

static Node *parse_return() {
  Node *node = new_node_expr(ND_RETURN, expr());

  Token *t = current_token();
  if (!consume(';') && t->ty != TK_EOF) {
    error("expected ';' : %s : pos = %d", t->input, pos);
  }

  return node;
}

static Node *parse_if() {
  trace_parse("if");

  expect('(');
  Node *cond = expr();
  expect(')');
  Node *then = stmt();

  Node *node = new_node_cond(ND_IF, cond);
  node->then = then;

  if (consume(TK_ELSE)) {
    node->els = stmt();
  }
  return node;
}

static Node *parse_while() {
  trace_parse("while");

  expect('(');
  Node *cond = expr();
  expect(')');
  Node *body = stmt();

  Node *node = new_node_cond(ND_WHILE, cond);
  node->body = body;

  return node;
}

static Node *parse_for() {
  trace_parse("for");

  Node *node = new_node(ND_FOR);
  expect('(');
  if (!consume(';')) {
    node->init = expr();
    expect(';');
  }

  if (!consume(';')) {
    node->cond = expr();
    expect(';');
  }

  if (!consume(')')) {
    node->inc = expr();
    expect(')');
  }

  node->body = stmt();

  return node;
}

static Node *parse_block() {
  trace_parse("block");

  Node *node = new_node(ND_BLOCK);
  node->stmts = new_vector();
  for (;;) {
    if (consume('}')) {
      break;
    }

    vec_push(node->stmts, stmt());
  }

  return node;
}

static Type *parse_typedef() {
  // check type defition
  Token *basetype = expect(TK_INT);

  if (strncmp(basetype->name, INT_TYPE, strlen(INT_TYPE))) {
    error("  invalid type : expected '%s', but got '%s' : pos = %d\n", INT_TYPE, basetype->name, pos);
  }

  Type *ty = &int_ty;

  // pointer type
  while (consume('*')) {
    ty = new_type(PTR, ty);
  }

  return ty;
}

static Node *function() {
  trace_parse("function");

  // parse type definition
  Type *ty = parse_typedef();

  Token *t = expect(TK_IDENT);

  Node *node = new_node(ND_FUNC);
  node->name = t->name;
  node->ty   = ty;
  node->scope = new_scope();
  scope = node->scope;

  // TODO: check duplicate function definition

  expect('(');

  if (!consume(')')) {
    // args
    Vector *args = new_vector();

    do {
      Type *argtype = parse_typedef();
      Token *t_ident = expect(TK_IDENT);
      vec_push(args, new_node_var_def(argtype, t_ident->name) );
    } while(consume(','));

    node->args = args;

    expect(')');
  }

  node->body = block();

  return node;
}

static Node *block() {
  trace_parse("block");

  expect('{');
  return parse_block();
}

static Node *stmt() {
  trace_parse("stmt");

  if (consume('{')) {
    return new_stmt(parse_block());
  }

  if (consume(TK_RETURN)) {
    return new_stmt(parse_return());
  }

  if (consume(TK_IF)) {
    return new_stmt(parse_if());
  }

  if (consume(TK_WHILE)) {
    return new_stmt(parse_while());
  }

  if (consume(TK_FOR)) {
    return new_stmt(parse_for());
  }

  // variable defition
  if (current_token()->ty == TK_INT) {
    Type *ty = parse_typedef();
    Token *t_ident = expect(TK_IDENT);
    expect(';');

    return new_stmt(new_node_var_def(ty, t_ident->name));
  }

  Node *node = expr();

  Token *t = current_token();
  if (!consume(';') && t->ty != TK_EOF) {
    error("expected ';' : %s : pos = %d", t->input, pos);
  }

  return new_stmt(node);
}

static Node *expr() {
  trace_parse("expr");

  return new_expr(assign());
}

static Node *assign() {
  trace_parse("assign");

  Node *node = equality();
  if (consume('=')) {
    Node *assign_node = new_node('=');
    assign_node->lhs = node;
    assign_node->rhs = new_expr(assign());
    node = assign_node;
  }

  return node;
}

static Node *equality() {
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

static Node *parse_num(Token *t) {
  return new_node_num(t->val);
}

static Node *parse_call(Token *t) {
  Node *node = new_node(ND_CALL);
  node->name = t->name;

  if (consume(')')) {
    return node;
  }

  Vector *args = new_vector();
  vec_push(args, expr());
  while(consume(',')) {
    vec_push(args, expr());
  }

  expect(')');
  node->args = args;

  return node;
}

static Node *term() {
  trace_parse("term");

  Token *t = current_token();

  // if next token is '(', "(" add ")" is expected
  if (consume('(')) {
    Node *node = expr();
    if (!consume(')')) {
      error("expected ')' : %s : pos = %d", t->input, pos);
    }
    return node;
  }

  // number
  if (t->ty == TK_NUM) {
    Node *node = parse_num(t);
    pos++;
    return node;
  }

  // identifier
  if (t->ty == TK_IDENT) {
    pos++;
    if (consume('(')) {
      return parse_call(t);
    }
    return new_node_var_ref(t->name);
  }

  error("term : invalid token: %s : pos = %d", t->input, pos);
  exit(1);
}

static Node *unary() {
  if (consume('+')) {
    return term();
  }
  if (consume('-')) {
    return new_node_bin_op('-', new_node_num(0), term());
  }
  if (consume('*')) {
    return new_deref(mul());
  }
  if (consume('&')) {
    return new_address_of(mul());
  }

  return term();
}
