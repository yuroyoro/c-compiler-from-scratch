#include "9cc.h"

static int roundup(int x, int align) {
  return (x + align - 1) & ~(align - 1);
}

static void gen_expr(Node *node) ;
static void gen_stmt(Node *node) ;

static void gen_num(Node *node) {
  printf("  push  %d\n", node->val);
}

static void gen_pop_binop_args() {
  printf("  pop   rdi\n");
  printf("  pop   rax\n");
}

static void gen_cmp(char *insn, Node *node) {
  dump_node("gen_cmp", node);
  printf("  cmp   rax, rdi\n");
  printf("  %s    al\n", insn);
  printf("  movzb rax, al\n");
}

static void gen_lval(Node *node) {
  dump_node("gen_lval", node);

  if (node->op != ND_VAR_REF) {
    error("invalid left value : %s", node->op);
  }

  printf("  mov   rax, rbp\n");
  printf("  sub   rax, %d\n", node->offset);
}

static void gen_load_mem() {
  printf("  mov   rax, [rax]\n");
}

static void gen_local_var(Node *node) {
  assert(node->op = ND_VAR_REF);

  gen_lval(node);
  gen_load_mem();
}

static void gen_deref(Node *node) {
  dump_node("gen_deref", node);
  assert(node->op = ND_DEREF);

  gen_expr(node->expr);

  // load expression result as address
  printf("  pop   rax\n");
  printf("  mov   rax, [rax]\n");
}

static void gen_return(Node *node) {
  dump_node("gen_return", node);
  gen_expr(node->expr);

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
  dump_node("gen_if", node);
  char *endlabel = gen_jump_label("end");

  // condition expr
  dump_node("gen_if : cond", node->cond);
  gen_expr(node->cond);

  // check condition result
  printf("  pop   rax\n");
  printf("  cmp   rax, 0\n");
  if (node->els == NULL) {
    printf("  je    %s\n", endlabel);

    dump_node("gen_if : then", node->then);
    gen_stmt(node->then);
  } else {
    char *elslabel = gen_jump_label("else");
    printf("  je    %s\n", elslabel);

    dump_node("gen_if : then", node->then);
    gen_stmt(node->then);
    printf("  jmp   %s\n", endlabel);
    printf("%s:\n", elslabel);

    dump_node("gen_if : els", node->els);
    gen_stmt(node->els);
  }

  printf("%s:\n", endlabel);
}

static void gen_while(Node *node) {
  dump_node("gen_while", node);

  char *beginlabel = gen_jump_label("begin");
  char *endlabel = gen_jump_label("end");

  printf("%s:\n", beginlabel);

  // condition expr
  dump_node("gen_while : cond", node->cond);
  gen_expr(node->cond);

  // check condition result
  printf("  pop   rax\n");
  printf("  cmp   rax, 0\n");
  printf("  je    %s\n", endlabel);

  // body
  dump_node("gen_while : body", node->cond);
  gen_stmt(node->body);
  printf("  jmp   %s\n", beginlabel);

  printf("%s:\n", endlabel);
}

static void gen_for(Node *node) {
  dump_node("gen_for", node->cond);

  char *beginlabel = gen_jump_label("begin");
  char *endlabel = gen_jump_label("end");

  // init expr
  if (node->init != NULL) {
    dump_node("gen_for : init", node->init);
    gen_expr(node->init);

    // discard init result
    printf("  # discard for init result\n");
    printf("  pop   rax\n");
  }

  printf("%s:\n", beginlabel);

  // condition expr
  if (node->cond != NULL) {
    dump_node("gen_for : cond", node->cond);

    gen_expr(node->cond);

    // check condition result
    printf("  pop   rax\n");
    printf("  cmp   rax, 0\n");
    printf("  je    %s\n", endlabel);
  }

  // body
  dump_node("gen_for : body", node->body);
  gen_stmt(node->body);

  // inc
  if (node->inc != NULL) {
    dump_node("gen_for : inc", node->inc);
    gen_expr(node->inc);

    // discard inc result
    printf("  # discard for inc result\n");
    printf("  pop   rax\n");
  }

  printf("  jmp   %s\n", beginlabel);
  printf("%s:\n", endlabel);
}

static void gen_block(Node *node) {
  dump_node("gen_block", node);

  for (int i = 0; i < node->stmts->len; i++) {
    Node *stmt = node->stmts->data[i];
    printf("  # gen_block : stmt %d : %-10s : op = %d, val = %d, name = [%s]\n", i, node_string(stmt->op), stmt->op, stmt->val, stmt->name);
    gen_stmt(stmt);
  }
}

char *FUNC_CALL_ARG_REGS[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};

static void gen_call(Node *node) {
  dump_node("gen_call", node);

  int argn = 0;
  // evaluate arguments
  if (node->args != NULL) {
    argn = node->args->len;
    for (int i = 0; i < node->args->len; i++) {
      Node *arg = (Node *)node->args->data[i];
      // evaluate
      printf("  # gen_call : arg %d : %-10s : op = %d, val = %d, name = [%s]\n", i, node_string(arg->op), arg->op, arg->val, arg->name);
      gen_expr(arg);
      // pop result and set register
      printf("  pop  %s\n", FUNC_CALL_ARG_REGS[i]);
    }
  }
  // push argument count
  printf("  mov   rax, %d\n", argn);

  // call
  printf("  call  %s\n", node->name);
}

static bool is_binop(Node *node) {
  return strchr("+-*/<", node->op) || node->op == ND_EQ ||
         node->op == ND_NE || node->op == ND_LE;
}

static void gen_assign(Node *node) {
  dump_node("gen_assign", node);

  dump_node("gen_assign : lhs", node->lhs);
  gen_lval(node->lhs);
  printf("  push  rax\n"); // push lhs addrees to stack

  dump_node("gen_assign : rhs", node->rhs);
  gen_expr(node->rhs);

  gen_pop_binop_args();
  printf("  mov   [rax], rdi\n");
  printf("  mov   rax, rdi\n");
}

static void gen_bin_op(Node *node) {
  dump_node("gen_bin_op", node);

  dump_node("gen_bin_op : lhs", node->lhs);
  gen_expr(node->lhs);
  dump_node("gen_bin_op : rhs", node->rhs);
  gen_expr(node->rhs);

  gen_pop_binop_args();

  switch (node->op) {
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
}

static void gen_header() {
  printf(".intel_syntax noprefix\n\n");
}

static void gen_func_header(char *name) {
  printf(".global %s\n ", name);
  printf("%s:\n", name);
}

static void gen_prologue(Node *node) {
  printf("  push  rbp\n");
  printf("  mov   rbp, rsp\n");
  // align stack pointer to 16byte
  printf("  sub   rsp, %d\n", roundup(node->scope->stacksize, 16));

  // TODO: save r11..r15 register to stack
}

static void gen_epilogue(Node *node) {
  printf("  mov   rsp, rbp\n");
  printf("  pop   rbp\n");
  printf("  ret\n\n");
}

static void gen_func(Node *node) {
  assert(node->op == ND_FUNC);

  gen_func_header(node->name);
  gen_prologue(node);

  // all args shold be identifier
  if (node->args != NULL) {
    for (int i = 0; i < node->args->len; i++) {
      Node *arg = node->args->data[i];
      printf("  # gen_func : arg %d : name = [%s], reg = %s\n", i, arg->name, FUNC_CALL_ARG_REGS[i]);
      printf("  mov   rax, rbp\n");
      printf("  sub   rax, %d\n", arg->offset);
      printf("  mov   [rax], %s\n", FUNC_CALL_ARG_REGS[i]);
    }
  }

  // body
  gen_block(node->body);

  gen_epilogue(node);
}

static void gen_expr(Node *node) {
  dump_node("gen_expr", node);

  assert(node->op == ND_EXPR);

  Node *expr = node->expr;

  switch(expr->op) {
    case ND_NUM :
      gen_num(expr);
      return; // num is directory pushed to stack;

    case ND_VAR_REF:
      gen_local_var(expr);
      break;

    case ND_CALL:
      gen_call(expr);
      break;

    case '=' :
      gen_assign(expr);
      break;

    case ND_DEREF:
      gen_deref(expr);
      break;

    default:
      if (is_binop(expr)) {
        gen_bin_op(expr) ;
        break;
      }

      error("code_gen : gen_expr : invalid node : %s\n", expr->op);
  }

  // push expression result
  printf("  # push expr result : %s\n", node_string(expr->op));
  printf("  push  rax\n");
}

static void gen_stmt(Node *node) {
  dump_node("gen_stmt", node);

  assert(node->op == ND_STMT);
  assert(node->body != NULL);

  dump_node("gen_stmt : body", node->body);
  Node *body = node->body;

  if (body->op == ND_BLOCK) {
    gen_block(body);
    return;
  }

  if (body->op == ND_IF) {
    gen_if(body);
    return;
  }

  if (body->op == ND_WHILE) {
    gen_while(body);
    return;
  }

  if (body->op == ND_FOR) {
    gen_for(body);
    return;
  }

  if (body->op == ND_RETURN) {
    gen_return(body);
    return;
  }

  if (body->op == ND_EXPR) {
    gen_expr(body);
    // discard expr result
    printf("  # discard expr result\n");
    printf("  pop   rax\n");
    return;
  }

  if (body->op == ND_VAR_DEF) {
    // noop
    return;
  }

  error("code_gen : gen_stmt : invalid node : %s\n", node_string(body->op));
}


void generate() {
  // print assembler headers
  gen_header();

  for (int i = 0; i < code->len; i++) {
    gen_func(code->data[i]);
  }
}
