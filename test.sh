#!/bin/bash

try(){
  expected="$1"
  input="$2"

  echo -n "$input => "
  ./9cc -debug "$input" > tmp.s
  local status=$?
  if [ $status -ne 0 ]; then
    echo "compilation failure : $status"
    exit $status
  fi

  gcc -o tmp tmp.s
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
try  0 0
try 42 42

# step 2 : + and - operator
try 21 '5+20-4'

# step 3 : tokenizer
try 41 " 12 + 34 - 5 "

# step 4 : *, / and parenthes
try 47 "5+6*7"
try 15 "5*(9-6)"
try  4 "(3+5)/2"

# step 5 : unary operator
try 30 "-3*+5*-2"
try  3 "-3 * -1"
try  5 "(-6+-4)/-2"
try 20 "(-6+-4)*-2"

# step 6 : equality and relational operator
try  1 "2 == 2"
try  0 "2 != 2"

try  1 "4 >= 3"
try  1 "3 >= 3"
try  0 "2 >= 3"

try  1 "4 > 3"
try  0 "3 > 3"
try  0 "2 > 3"

try  1 "3 <= 4"
try  1 "3 <= 3"
try  0 "3 <= 2"

try  1 "3 < 4"
try  0 "3 < 3"
try  0 "3 < 2"

# step 9: variable, assign and statements
try 42 "42;"
try  4 "a=b=2;a+b;"
try  1 "a = 3; b = a + 4; (a + 3) / 2 < a + b;"

# step 10: return statement
try 42 "return 42;"
try  4 "a=b=2;return a+b;"
code=$(cat <<EOF
  a = 3;
  b = 4;
  c = a * b - 2;
  return c;
EOF
)

try 10 "$code"

# step 11: multi letter variable
try  8 "a=4;aa=2;abc123_def=a*aa; return abc123_def;"

code=$(cat <<EOF
  foo = 3;
  bar = 4;
  baz = foo * bar - 2;
  return baz;
EOF
)

try 10 "$code"

# step 12 : control statement

# if
try 2 "a = 1; if (a > 0) a = 2; return a"
try 1 "a = 1; if (a < 0) a = 2; return a"

code=$(cat <<EOF
  a = 1;
  if (a > 0)
    if (a == 1)
      return 1;
  return 2;
EOF
)

try 1 "$code"

# else
try 2 "a = 1; if (a > 0) a = 2; else a = 3; return a"
try 3 "a = 1; if (a < 0) a = 2; else a = 3; return a"

code=$(cat <<EOF
  a = 1;
  if (a > 0)
    if (a != 1)
      return 1;
    else
      return 3;
  return 2;
EOF
)

try 3 "$code"

# while
try 10 "a = 1; while (a < 10) a = a + 1; return a"

code=$(cat <<EOF
  a = 1;
  while (a < 10)
    if (a != 9)
      a = a + 1;
    else
      return a = 99;
  return a;
EOF
)

try 99 "$code"

# for
try 10 "a = 1; for (i = 0; i < 10; i = i + 1) a = i;"
try 10 "a = 1; for (; a < 10;) a = a + 1;"

# end test
echo OK
