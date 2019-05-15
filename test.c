#include "9cc.h"

// unit test for Vector
static void expect(int line, int expected, int actual) {
  if (expected == actual) {
    return;
  }

  fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
  exit(1);
}

void test_vector() {
  Vector *vec = new_vector();
  expect(__LINE__, 0, vec->len);

  for (int i = 0; i < 100; i++) {
    vec_push(vec, (void *)(long)i);
  }

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, (long)vec->data[0]);
  expect(__LINE__, 50, (long)vec->data[50]);
  expect(__LINE__, 99, (long)vec->data[99]);
}

void test_map() {
  Map *map = new_map();
  expect(__LINE__, 0, (long)map_get(map, "foo"));

  map_put(map, "foo", (void *)2);
  expect(__LINE__, 2, (long)map_get(map, "foo"));

  map_put(map, "bar", (void *)4);
  expect(__LINE__, 4, (long)map_get(map, "bar"));

  map_put(map, "foo", (void *)6);
  expect(__LINE__, 6, (long)map_get(map, "foo"));
}

void runtest() {
  test_vector();
  test_map();
}
