#include <string.h>

int main() {
  char buf1[] = "short", buf2[] = "longlongstring";
  char *p;
  p = buf2;
  p = buf1;
  p[strlen(p)+1] = '\0';
}
