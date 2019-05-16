#!/bin/bash

try(){
  expected="$1"
  input="$2"

  ./9cc -debug "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
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

echo OK
