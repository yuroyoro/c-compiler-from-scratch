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

  program();

  // generate assembler codes by traversing AST
  generate();

  return 0;
}
