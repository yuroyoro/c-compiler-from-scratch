#!/bin/bash

try(){
  expected="$1"
  input="$2"
  obj="$3"

  echo -n "$input => "
  ./9cc -debug "$input" > tmp.s
  local status=$?
  if [ $status -ne 0 ]; then
    echo "compilation failure : $status"
    exit $status
  fi

  if [ -n "$obj" ]; then
    gcc -static -o tmp tmp.s "$obj"
  else
    gcc -o tmp tmp.s
  fi

  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

# step 1 : number
# try  0 0
# try 42 42

# step 2 : + and - operator
# try 21 '5+20-4'

# step 3 : tokenizer
# try 41 " 12 + 34 - 5 "

# step 4 : *, / and parenthes
# try 47 "5+6*7"
# try 15 "5*(9-6)"
# try  4 "(3+5)/2"

# step 5 : unary operator
try 30 "int main() { return -3*+5*-2; } "
# try  3 "-3 * -1"
try  5 "int main() { return (-6+-4)/-2; }"
# try 20 "(-6+-4)*-2"

# step 6 : equality and relational operator
try  8 "int main() { return (2 == 2) + (2 == 2) + (4 >= 3) + (3 >= 3) + (4 > 3) + (3 <= 4) + (3 <= 3) + (3 < 4) ; }"
try  0 "int main() { return (2 != 2) + (2 >= 3) + (3 > 3) + (2 > 3) + (3 <= 2) + (3 < 3) + (3 < 2); }"

# step 9: variable, assign and statements
# try  4 "a=b=2;a+b;"
# try  1 "a = 3; b = a + 4; (a + 3) / 2 < a + b;"

# step 10: return statement
# try 42 "return 42;"
# try  4 "a=b=2;return a+b;"
code=$(cat <<EOF
int main() {
  int a; int b; int c;
  a = 3;
  b = 4;
  c = a * b - 2;
  return c;
}
EOF
)

try 10 "$code"

# step 11: multi letter variable
# try  8 "a=4;aa=2;abc123_def=a*aa; return abc123_def;"

code=$(cat <<EOF
int main() {
  int foo; int bar; int baz;
  foo = 3;
  bar = 4;
  baz = foo * bar - 2;
  return baz;
}
EOF
)

try 10 "$code"

# step 12 : control statement

# if
try 2 "int main() { int a ;a = 1; if (a > 0) a = 2; return a; }"
try 1 "int main() { int a ;a = 1; if (a < 0) a = 2; return a; }"

code=$(cat <<EOF
int main() {
  int a ;a = 1;
  if (a > 0)
    if (a == 1)
      return 1;
  return 2;
}
EOF
)

try 1 "$code"

# else
try 2 "int main() {int a ;a = 1; if (a > 0) a = 2; else a = 3; return a; }"
try 3 "int main() {int a ;a = 1; if (a < 0) a = 2; else a = 3; return a; }"

code=$(cat <<EOF
int main() {
  int a ;a = 1;
  if (a > 0)
    if (a != 1)
      return 1;
    else
      return 3;
  return 2;
}
EOF
)

try 3 "$code"

# while
try 10 "int main() {int a ;a = 1; while (a < 10) a = a + 1; return a; }"

code=$(cat <<EOF
int main() {
  int a ;a = 1;
  while (a < 10)
    if (a != 9)
      a = a + 1;
    else
      return a = 99;
  return a;
}
EOF
)

try 99 "$code"

# for
try  9 "int main() {int a ; int i; a = 1; for (i = 0; i < 10; i = i + 1) a = i; return a;}"
try 10 "int main() {int a ; int i; a = 1; for (; a < 10;) a = a + 1; return a;}"

# step 13 : statements block

try  2 "int main() { int a; a = 1; a = a + 1; return a; }"

code=$(cat <<EOF
int main() {
  int a;
  a = 1;
  if (a != 1) {
    a = a + 1;
    return a;
  } else {
    a = a + 2;
    return a;
  }
}
EOF
)

try 3 "$code"

code=$(cat <<EOF
int main() {
  int a; int b;
  a = 1;
  b = 10;
  while (a < 10) {
    a = a + 1;
    b = b + 10;
  }
  return a + b;
}
EOF
)

try 110  "$code"

code=$(cat <<EOF
int main() {
  int a; int b;
  a = 1;
  b = 10;
  for (a = 1; a < 10; a = a + 1) {
    b = b + 10;
  }
  return a + b;
}
EOF
)

try 110  "$code"

# step 14 : function call

echo 'foo() { return 99; }' | gcc -xc -c -o tmp-foo.o -
try 99 "int main() { return foo(); }" tmp-foo.o

echo 'add(x, y) { return x+y; }' | gcc -xc -c -o tmp-add.o -
try 14 "int main() { int a;a =10; return add(a, 4); }" tmp-add.o

# step 15 : function definition

code=$(cat <<EOF
int foo(int a, int b) {
  return a+b;
}

int main() {
  int a;
  a = 1;
  return foo(a, 2);
}
EOF
)

try 3  "$code"

code=$(cat <<EOF
int foo(int a, int b) { return a + b; }
int bar(int n) { return baz(n) + 1; }
int baz(int n) { return n * 2; }

int main() {
  int a;
  a = 1;
  return foo(a, bar(2));
}
EOF
)

try 6  "$code"

# step 17: pointer
try 99 "int main(){ int *p; int **pp; return 99; }"

code=$(cat <<EOF
#include <stdlib.h>
int *intptr() { int *p = calloc(1, sizeof(int)); *p = 99; return p; }
int deref(int *p) { return *p; }
EOF
)

echo "$code" | gcc -xc -c -o tmp-intptr.o -

try 99 "int main() { int *p; p = intptr(); return deref(p); }" tmp-intptr.o

# deereference
try 99 "int main() { int *p; p = intptr(); return *p; }" tmp-intptr.o
# address-of
try 99 "int main() { int a; int *p; a = 99; p = &a; return *p; }"
# pointer of pointer type
try 99 "int main() { int a; int *p1; int **p2; a = 99; p1 = &a; p2 = &p1; return **p2; }"

# end test
echo OK
