#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tokens
enum {
  TK_NUM = 256, // number
  TK_EOF,       // terminator
};


typedef struct {
  int ty;       // type of Tokens
  int val;      // number value if ty == TK_NUM
  char *input;  // token string (for error messsage)
} Token;

// put result of tgkenized tokens into bellow.
// we assume that do not input over 100 tokens
Token tokens[100];

// error reporting fuction
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// split string that p pointed to token, and store them to tokens
void tokenize(char *p) {
  int i = 0;
  while(*p) {
    // skip white spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (*p == '+' || *p == '-') {
      tokens[i].ty = *p;
      tokens[i].input = p;
      i++;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p, &p, 10);
      i++;
      continue;
    }

    error("could not tokenize: %s", p);
    exit(1);
  }

  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "invalid argument count\n");
    return 1;
  }

  // tokenize input
  tokenize(argv[1]);

  // print assembler headers
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // The first token must be number
  if (tokens[0].ty != TK_NUM)
    error("the first expression must be number");
  printf("  mov rax, %d\n", tokens[0].val);

  int i = 1;
  while (tokens[i].ty != TK_EOF) {
    if (tokens[i].ty == '+') {
      i++;
      if (tokens[i].ty != TK_NUM)
        error("unexpected token: %s", tokens[i].input);
      printf("  add rax, %d\n", tokens[i].val);
      i++;
      continue;
    }

    if (tokens[i].ty == '-') {
      i++;
      if (tokens[i].ty != TK_NUM)
        error("unexpected token: %s", tokens[i].input);
      printf("  sub rax, %d\n", tokens[i].val);
      i++;
      continue;
    }

    error("unexpected token: %s", tokens[i].input);
  }

  printf("  ret\n");

  return 0;
}
