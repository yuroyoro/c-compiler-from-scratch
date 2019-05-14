#include "9cc.h"

static void gen(Node *node) ;

static void gen_num(Node *node) {
  printf("  # node num %d\n", node->val);
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

  int offset = ('z' - node->name + 1) * 8;
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
  gen(node->lhs);
  printf("  pop   rax\n");
  printf("  mov   rsp, rbp\n");
  printf("  pop   rbp\n");
  printf("  ret\n");
}

static void gen_header() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

static void gen_prologue() {
  printf("  push  rbp\n");
  printf("  mov   rbp, rsp\n");
  printf("  sub   rsp, %d\n", 26 * 8);

}

static void gen_epilogue() {
  printf("  mov   rsp, rbp\n");
  printf("  pop   rbp\n");
  printf("  ret\n");
}

// code generator
static void gen(Node *node) {
  printf("  # node %d\n", node->ty);

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
