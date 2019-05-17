#include "9cc.h"

static void gen(Node *node) ;

static void gen_num(Node *node) {
  printf("  push  %d\n", node->val);
}

static void gen_pop_stack() {
  printf("  pop   rdi\n");
  printf("  pop   rax\n");
}

static void gen_push_stack(Node *node) {
  printf("  push  rax\n");
}

static void gen_cmp(char *insn, Node *node) {
  printf("  cmp   rax, rdi\n");
  printf("  %s    al\n", insn);
  printf("  movzb rax, al\n");
}

static void gen_lval(Node *node) {
  if (node->ty != TK_IDENT) {
    error("invalid left value : %s", node->ty);
  }

  int offset = (map_geti(vars, node->name)) * 8;
  printf("  mov   rax, rbp\n");
  printf("  sub   rax, %d\n", offset);
  printf("  push  rax\n");
}

static void gen_load_mem() {
  printf("  pop   rax\n");
  printf("  mov   rax, [rax]\n");
  printf("  push  rax\n");
}

static void gen_return(Node *node) {
  gen(node->expr);
  printf("  pop   rax\n");
  printf("  mov   rsp, rbp\n");
  printf("  pop   rbp\n");
  printf("  ret\n");
}

#define MAX_LABEL_COUNT 10000
int label_cnt = 0;
static char *gen_jump_label(char *name) {
  if (label_cnt >= MAX_LABEL_COUNT) {
    error("too many jump label\n");
  }

  int len = strlen(name) + 7;
  char *str = malloc(sizeof(char) * len);
  sprintf(str, ".L%s%04d", name, label_cnt++);
  return str;
}

static void gen_if(Node *node) {
  char *endlabel = gen_jump_label("end");

  // condition expr
  gen(node->cond);
  // check condition result
  printf("  pop   rax\n");
  printf("  cmp   rax, 0\n");
  if (node->els == NULL) {
    printf("  je    %s\n", endlabel);
    gen(node->expr);
  } else {
    char *elslabel = gen_jump_label("else");
    printf("  je    %s\n", elslabel);
    gen(node->expr);
    printf("  jmp   %s\n", endlabel);
    printf("%s:\n", elslabel);
    gen(node->els);
  }

  printf("%s:\n", endlabel);
}

static bool is_binop(Node *node) {
  return strchr("+-*/<", node->ty) || node->ty == ND_EQ ||
         node->ty == ND_NE || node->ty == ND_LE;
}

static void gen_bin_op(Node *node) {
  gen(node->lhs);
  gen(node->rhs);

  gen_pop_stack();

  switch (node->ty) {
    case '=':
      break;
    case ND_EQ:
      gen_cmp("sete", node);
      break;
    case ND_NE:
      gen_cmp("setne", node);
      break;
    case ND_LE:
      gen_cmp("setle", node);
      break;
    case '<':
      gen_cmp("setl", node);
      break;
    case '+':
      printf("  add   rax, rdi\n");
      break;
    case '-':
      printf("  sub   rax, rdi\n");
      break;
    case '*':
      printf("  mul   rdi\n");
      break;
    case '/':
      printf("  cqo\n");
      printf("  idiv  rdi\n");
      break;
  }

  gen_push_stack(node);
}

static void gen_header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

static void gen_prologue() {
  printf("  push  rbp\n");
  printf("  mov   rbp, rsp\n");
  printf("  sub   rsp, %d\n", var_cnt * 8);
}

static void gen_epilogue() {
  printf("  mov   rsp, rbp\n");
  printf("  pop   rbp\n");
  printf("  ret\n");
}

// code generator
static void gen(Node *node) {
  if (debug) {
    dump_node(node);
  }

  if (node->ty == ND_IF) {
    gen_if(node);
    return;
  }

  if (node->ty == ND_RETURN) {
    gen_return(node);
    return;
  }

  if (node->ty == ND_NUM) {
    gen_num(node);
    return;
  }

  if (node->ty == ND_IDENT) {
    gen_lval(node);
    gen_load_mem();
    return;
  }

  if (node->ty == '=') {
    gen_lval(node->lhs);
    gen(node->rhs);

    gen_pop_stack();
    printf("  mov   [rax], rdi\n");
    printf("  push  rdi\n");

    return;
  }

  if (is_binop(node)) {
    gen_bin_op(node) ;
    return;
  }

  error("unknown node type : %d", node->ty);
}

void generate() {
  // print assembler headers
  gen_header();

  gen_prologue();
  for (int i = 0; code[i]; i++) {
    gen(code[i]);

    // pop result of last evaluated expression
    printf("  pop   rax\n");
  }

  gen_epilogue();
}
