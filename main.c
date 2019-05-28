#include "9cc.h"

// error reporting fuction
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// warning reporting function
void warn(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stdout, "# [warn] ");
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
}

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

  if (debug) {
    for(int i = 0; i < tokens->len; i++) {
      Token *t = (Token *)tokens->data[i];
      dump_token(i, t);
    }
  }

  program();

  if (debug) {
    dump_nodes();
  }

  // generate assembler codes by traversing AST
  generate();

  return 0;
}
