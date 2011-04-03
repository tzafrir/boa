#include <string.h>

int main() {
  char buf1[] = "short", buf2[] = "longlongstring";
  char *p;
  int len;
  p = buf2;
  len = strlen(p) + 1;
  p = buf1;
  p[len] = '\0';
}
