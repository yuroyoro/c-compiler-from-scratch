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

static bool is_alnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') ||
         (c == '_');
}

static bool is_keyword(char *s1, char *s2) {
  return startswith(s1, s2) && !is_alnum(s1[strlen(s2)]);
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

    // multi charcter operator
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

    // single charcter operator
    if (strchr("+-*/()<>;=", *p)) {
      Token *token = new_token(*p, p);
      vec_push(vec, (void *)token);
      p++;
      continue;
    }

    // return
    if (is_keyword(p, "return")) {
      Token *token = new_token(TK_RETURN, p);
      vec_push(vec, (void *)token);
      p += 6;
      continue;
    }

    // number
    if (isdigit(*p)) {
      Token *token = new_token(TK_NUM, p);
      vec_push(vec, (void *)token);
      token->val = strtol(p, &p, 10);
      continue;
    }

    // identifier
    if ('a' <= *p && *p <= 'z') {
      Token *token = new_token(TK_IDENT, p);
      vec_push(vec, (void *)token);
      p++;
      continue;
    }

    error("could not tokenize: %s", p);
    exit(1);
  }

  Token *token = new_token(TK_EOF, p);
  vec_push(vec, (void *)token);

  return vec;
}
