#include "9cc.h"

const char *TOKEN_STRING[] = {
  STRING(TK_NUM),
  STRING(TK_IDENT),
  STRING(TK_RETURN),
  STRING(TK_IF),
  STRING(TK_ELSE),
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


Map *keywords;

Map *keyword_map() {
  Map *map = new_map();

  map_puti(map, "return", TK_RETURN);
  map_puti(map, "if",     TK_IF);
  map_puti(map, "else",   TK_ELSE);

  return map;
}

static bool startswith(char *s1, char *s2) {
  return !strncmp(s1, s2, strlen(s2));
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
  char *str = *p;

  // first charcter should be alpah or _
  if (!isalpha(*str) && *str != '_') {
    return false;
  }

  // read input while appah or digit or _
  int i = 1;
  while (isalpha(str[i]) || isdigit(str[i]) || str[i] == '_') {
    i++;
  }

  char *name = strndup(str, i);

  // check keyword
  int ty = map_geti(keywords, name);
  if ((void *)(intptr_t)ty == NULL) {
    ty = TK_IDENT;
  }

  Token *token = new_token(ty, name);
  token->name = name;

  vec_push(vec, (void *)token);
  *p = str + i;
  return true;
}

// split string that p pointed to token, and store them to Vector
Vector *tokenize(char *p) {
  Vector *vec = new_vector();
  keywords = keyword_map();

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

    // identifier
    if (identifier(vec, &p)) {
      continue;
    }

    // number
    if (number(vec, &p)) {
      continue;
    }

    error("could not tokenize: %s", p);
    exit(1);
  }

  Token *token = new_token(TK_EOF, p);
  vec_push(vec, (void *)token);

  return vec;
}
