#include "9cc.h"

const char *TOKEN_STRING[] = {
  STRING(TK_NUM),
  STRING(TK_IDENT),
  STRING(TK_RETURN),
  STRING(TK_EQ),
  STRING(TK_NE),
  STRING(TK_LE),
  STRING(TK_GE),
  STRING(TK_EOF),
};

char *token_string(int ty) {
  char *str;
  if (ty < 256) {
    str = malloc(sizeof(char) * 2);
    sprintf(str, "%c", ty);
    return str;
  }

  str = strndup(TOKEN_STRING[ty - 256], strlen(TOKEN_STRING[ty - 256]));
  return str;
}

void dump_token(int i, Token *t) {
  printf("# token %2d : %-10s : ty = %d, val = %d, input = [%s]\n", i, token_string(t->ty), t->ty, t->val, t->input);
}

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
    Token *token = new_token(**p, strndup(*p, 1));
    vec_push(vec, (void *)token);
    *p = (char *)*p+1;
    return true;
  }

  return false;
}

static bool number(Vector *vec, char **p) {
  if (isdigit(**p)) {
    char *old_p = *p;
    int val = strtol(*p, &*p, 10);

    Token *token = new_token(TK_NUM, strndup(old_p, *p - old_p));
    vec_push(vec, (void *)token);
    token->val = val;
    return true;
  }

  return false;
}

static bool identifier(Vector *vec, char **p) {
  if (!isalpha(**p) && **p != '_') {
    return false;
  }

  Token *token = new_token(TK_IDENT, strndup(*p, 1));
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
      Token *token = new_token(TK_RETURN, "return");
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
