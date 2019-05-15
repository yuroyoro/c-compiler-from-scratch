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
static bool space(Vector *vec, char **p) {
  if (isspace(**p)) {
    *p = (char *)*p+1;
    return true;
  }
  return false;
}

static bool multi_character_opeator(Vector *vec, char **p) {
  for (int i = 0; symbols[i].name; i++) {
    char *name = symbols[i].name;
    if (!startswith(*p, name)) {
      continue;
    }

    Token *token = new_token(symbols[i].ty, name);
    vec_push(vec, (void *)token);

    *p += strlen(name);
    return true;
  }

  return false;
}

static bool single_character_opeator(Vector *vec, char **p) {
  if (strchr("+-*/()<>;=", **p)) {
    Token *token = new_token(**p, *p);
    vec_push(vec, (void *)token);
    *p = (char *)*p+1;
    return true;
  }

  return false;
}

static bool number(Vector *vec, char **p) {
  if (isdigit(**p)) {
    Token *token = new_token(TK_NUM, *p);
    vec_push(vec, (void *)token);
    token->val = strtol(*p, &*p, 10);
    return true;
  }

  return false;
}

static bool identifier(Vector *vec, char **p) {
  if (!isalpha(**p) && **p != '_') {
    return false;
  }

  Token *token = new_token(TK_IDENT, *p);
  vec_push(vec, (void *)token);
  *p = (char *)*p+1;
  return true;
}

// split string that p pointed to token, and store them to Vector
Vector *tokenize(char *p) {
  Vector *vec = new_vector();

  while(*p) {
    // skip white spaces
    if (space(vec, &p)) {
      continue;
    }

    // multi charcter operator
    if (multi_character_opeator(vec, &p)) {
      continue;
    }

    // single charcter operator
    if (single_character_opeator(vec, &p)) {
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
    if (number(vec, &p)) {
      continue;
    }

    // identifier
    if (identifier(vec, &p)) {
      continue;
    }

    error("could not tokenize: %s", p);
    exit(1);
  }

  Token *token = new_token(TK_EOF, p);
  vec_push(vec, (void *)token);

  return vec;
}
