#include "string.h"

int main() {
  char buf1[] = "hello", buf2[] = "world";
  char *p1 = strpbrk(buf1, "lo");
  char *p2 = strpbrk(buf2, "d");
  p1[0] = 'a';
  p2[1] = 'a';
}
