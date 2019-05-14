#include "9cc.h"

static Token *new_token(int ty, char *input) {
  Token *token = malloc(sizeof(Token));
  token->ty = ty;
  token->input = input;
  return token;
}

static struct {
  char *name;
  int ty;
} symbols[] = {
    {"!=", TK_NE}, {"<=", TK_LE},
    {"==", TK_EQ}, {">=", TK_GE},
    {NULL, 0},
};

static bool startswith(char *s1, char *s2) {
  return !strncmp(s1, s2, strlen(s2));
}

// split string that p pointed to token, and store them to Vector
Vector *tokenize(char *p) {
  Vector *vec = new_vector();

  while(*p) {
    // skip white spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    bool found = false;
    for (int i = 0; symbols[i].name; i++) {
      char *name = symbols[i].name;
      if (!startswith(p, name)) {
        continue;
      }

      Token *token = new_token(symbols[i].ty, name);
      vec_push(vec, (void *)token);

      p += strlen(name);
      found = true;
      break;
    }
    if (found) {
      continue;
    }

    if (strchr("+-*/()<>", *p)) {
      Token *token = new_token(*p, p);
      vec_push(vec, (void *)token);
      p++;
      continue;
    }

    if (isdigit(*p)) {
      Token *token = new_token(TK_NUM, p);
      vec_push(vec, (void *)token);
      token->val = strtol(p, &p, 10);
      continue;
    }

    error("could not tokenize: %s", p);
    exit(1);
  }

  Token *token = new_token(TK_EOF, p);
  vec_push(vec, (void *)token);

  return vec;
}
