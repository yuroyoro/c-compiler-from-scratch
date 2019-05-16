#include "9cc.h"

// error reporting fuction
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

bool debug = false;

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "invalid argument count\n");
    return 1;
  }

  if (!strncmp(argv[1], "-test", 5)) {
    runtest();
    return 0;
  }

  int argpos = 1;
  char *code;

  if (!strncmp(argv[argpos], "-debug", 6)) {
    debug = true;
    argpos++;
  }

  code = argv[argpos];

  // tokenize input
  tokens = tokenize(code);

  for(int i = 0; i < tokens->len; i++) {
    Token *t = (Token *)tokens->data[i];
    if (debug) {
      dump_token(i, t);
    }
  }

  program();

  // generate assembler codes by traversing AST
  generate();

  return 0;
}
