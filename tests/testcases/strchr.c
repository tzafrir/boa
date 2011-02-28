#include "string.h"

int main() {
  char buf1[] = "hello", buf2[] = "world";
  char *p1 = strchr(buf1, 'o');
  char *p2 = strchr(buf2, '\0');
  p1[0] = 'a';
  p2[1] = 'a';
}
