#include "9cc.h"

bool debug = false;

void dump_token(int i, Token *t) {
  printf("# token %2d : %-10s : ty = %d, val = %d, input = [%s]\n", i, token_string(t->ty), t->ty, t->val, t->input);
}

void dump_node(char *msg, Node *n) {
  if (debug) {
    printf("  # %-10s : node %-10s : op = %d, val = %d, name = [%s]\n", msg, node_string(n->op), n->op, n->val, n->name);
  }
}

void print_node(Node *n, char *msg, int depth) {
  char sp[depth+1];
  for (int i = 0; i < depth; i++) {
    sp[i] = ' ';
  }
  sp[depth] = '\0';

  printf("# %s- %-10s : node %-10s : op = %d, val = %d, name = [%s]\n", sp, msg, node_string(n->op), n->op, n->val, n->name);
  if (n->ty != NULL) {
    printf("# %s- %-10s : type %d\n", sp, msg, n->ty->ty);
  }

  if (n->lhs != NULL) {
    print_node(n->lhs, "lhs", depth+4);
  }
  if (n->rhs != NULL) {
    print_node(n->rhs, "rhs", depth+4);
  }
  if (n->expr != NULL) {
    print_node(n->expr, "expr", depth+4);
  }
  if (n->cond != NULL) {
    print_node(n->cond, "cond", depth+4);
  }
  if (n->then != NULL) {
    print_node(n->then, "then", depth+4);
  }
  if (n->els != NULL) {
    print_node(n->els, "els", depth+4);
  }
  if (n->body != NULL) {
    print_node(n->body, "body", depth+4);
  }
  if (n->init != NULL) {
    print_node(n->init, "init", depth+4);
  }
  if (n->inc != NULL) {
    print_node(n->inc, "inc", depth+4);
  }

  if (n->stmts != NULL) {
    for (int i = 0; i < n->stmts->len ; i++) {
      print_node(n->stmts->data[i], "stmts", depth+4);
    }
  }

  if (n->args != NULL) {
    for (int i = 0; i < n->args->len ; i++) {
      print_node(n->args->data[i], "args", depth+4);
    }
  }
}

void dump_nodes() {
  printf("# dump nodes\n\n");
  for (int i = 0; i < code->len; i++) {
    print_node(code->data[i], "func", 0);
  }
  printf("# \n");
}
