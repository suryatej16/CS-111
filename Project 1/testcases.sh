#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
a > b | c

a       b

a;c;d

a&&b||

c||d

g<e

(a||b);(c&&d)

a && (b || d) | c && e ; f < g > h
EOF

cat >test.exp <<'EOF'
# 1
    a>b \
  |
    c
# 2
  a b
# 3
    a \
  ;
    c \
  ;
    d
# 4
      a \
    &&
      b \
  ||
    c \
  ||
    d
# 5
  g<e
# 6
    (
       a \
     ||
       b
    ) \
  ;
    (
       c \
     &&
       d
    )
# 7
      a \
    &&
        (
           b \
         ||
           d
        ) \
      |
        c \
    &&
      e \
  ;
    f<g>h
EOF

../timetrash -p test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
