#include "9cc.h"

static void gen(Node *node) ;

static int stackpos;

static void gen_pop(char *dst) {
  printf("  pop   %s\n", dst );
  stackpos -= 8;
}

static void gen_push(char *src) {
  printf("  push  %s\n", src);
  stackpos += 8;
}

static void gen_num(Node *node) {
  printf("  push  %d\n", node->val);
  stackpos += 8;
}

static void gen_pop_binop_args() {
  gen_pop("rdi");
  gen_pop("rax");
}

static void gen_push_stack(Node *node) {
  gen_push("rax");
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
  gen_push("rax");
}

static void gen_load_mem() {
  gen_pop("rax");
  printf("  mov   rax, [rax]\n");
  gen_push("rax");
}

static void gen_return(Node *node) {
  gen(node->expr);
  gen_pop("rax");
  printf("  mov   rsp, rbp\n");
  gen_pop("rbp");
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
  gen_pop("rax");
  printf("  cmp   rax, 0\n");
  if (node->els == NULL) {
    printf("  je    %s\n", endlabel);
    gen(node->then);
  } else {
    char *elslabel = gen_jump_label("else");
    printf("  je    %s\n", elslabel);
    gen(node->then);
    printf("  jmp   %s\n", endlabel);
    printf("%s:\n", elslabel);
    gen(node->els);
  }

  printf("%s:\n", endlabel);
}

static void gen_while(Node *node) {
  char *beginlabel = gen_jump_label("begin");
  char *endlabel = gen_jump_label("end");

  printf("%s:\n", beginlabel);

  // condition expr
  gen(node->cond);
  // check condition result
  gen_pop("rax");
  printf("  cmp   rax, 0\n");
  printf("  je    %s\n", endlabel);
  // body
  gen(node->body);
  printf("  jmp   %s\n", beginlabel);

  printf("%s:\n", endlabel);
}

static void gen_for(Node *node) {
  char *beginlabel = gen_jump_label("begin");
  char *endlabel = gen_jump_label("end");

  // init expr
  if (node->init != NULL) {
    gen(node->init);
  }

  printf("%s:\n", beginlabel);

  // condition expr
  if (node->cond != NULL) {
    gen(node->cond);
    // check condition result
    gen_pop("rax");
    printf("  cmp   rax, 0\n");
    printf("  je    %s\n", endlabel);
  }

  // body
  gen(node->body);

  // inc
  if (node->inc != NULL) {
    gen(node->inc);
  }

  printf("  jmp   %s\n", beginlabel);
  printf("%s:\n", endlabel);
}

static void gen_block(Node *node) {
  for (int i = 0; i < node->stmts->len; i++) {
    gen(node->stmts->data[i]);
    gen_pop("rax"); // pop stmt result
  }
}

char *FUNC_CALL_ARG_REGS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};

static void gen_call(Node *node) {
  int argn = 0;
  // evaluate arguments
  if (node->args != NULL) {
    argn = node->args->len;
    for (int i = 0; i < node->args->len; i++) {
      Node *arg = (Node *)node->args->data[i];
      // evaluate
      printf("  # arg %d : %-10s : ty = %d, val = %d, name = [%s]\n", i, node_string(arg->ty), arg->ty, arg->val, arg->name);
      gen(arg);
      // pop result and set register
      gen_pop(FUNC_CALL_ARG_REGS[i]);
    }
  }
  // push argument count
  printf("  mov   rax, %d\n", argn);

  // adjust stackp position to 16byte aligned
  bool padding = stackpos % 16;
  if (padding) {
    printf("  sub rsp, 8\n");
    stackpos += 8;
  }

  // call
  printf("  call  %s\n", node->name);

  if (padding) {
    printf("  add rsp, 8\n");
    stackpos -= 8;
  }

  // push result to stack
  gen_push("rax");
}

static bool is_binop(Node *node) {
  return strchr("+-*/<", node->ty) || node->ty == ND_EQ ||
         node->ty == ND_NE || node->ty == ND_LE;
}

static void gen_assign(Node *node) {
  gen_lval(node->lhs);
  gen(node->rhs);

  gen_pop_binop_args();
  printf("  mov   [rax], rdi\n");
  gen_push("rdi");
}


static void gen_bin_op(Node *node) {
  gen(node->lhs);
  gen(node->rhs);

  gen_pop_binop_args();

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
  gen_push("rbp");
  printf("  mov   rbp, rsp\n");
  printf("  sub   rsp, %d\n", var_cnt * 8);
}

static void gen_epilogue() {
  printf("  mov   rsp, rbp\n");
  gen_pop("rbp");
  printf("  ret\n");
}

// code generator
static void gen(Node *node) {
  if (debug) {
    dump_node(node);
  }

  if (node->ty == ND_BLOCK) {
    gen_block(node);
    return;
  }

  if (node->ty == ND_IF) {
    gen_if(node);
    return;
  }

  if (node->ty == ND_WHILE) {
    gen_while(node);
    return;
  }

  if (node->ty == ND_FOR) {
    gen_for(node);
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

  if (node->ty == ND_CALL) {
    gen_call(node);
    return;
  }

  if (node->ty == '=') {
    gen_assign(node);
    return;
  }

  if (is_binop(node)) {
    gen_bin_op(node) ;
    return;
  }

  error("unknown node type : %d", node->ty);
}

void generate() {
  stackpos = 8;
  // print assembler headers
  gen_header();

  gen_prologue();
  for (int i = 0; code[i]; i++) {
    gen(code[i]);

    // pop result of last evaluated expression
    gen_pop("rax");
  }

  gen_epilogue();
}
