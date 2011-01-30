// BOA-TEST   1
// simple malloc
#include <stdlib.h>

int main() {
  char *p, *q, *r;
  int i = 4;
  p = (char*)malloc(i);
  p[5] = 'a';
  q = (char*)malloc(i);
  q[4] = 'b';
  r = (char*)malloc(i - 1);
  r[4] = 'c';
  return 0;
}

