#include "stdlib.h"

int main() {
  char *b1 = malloc(10);
  char *b2 = malloc(20);
  char **pp1 = &b1;
  char **pp2 = &b2;
  char *p1 = (*pp1) + 10;
  char *p2 = (*pp2) + 10;
  p1[0] = 'a';
  p2[0] = 'a';
}
