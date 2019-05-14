#include "9cc.h"

// error reporting fuction
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
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
