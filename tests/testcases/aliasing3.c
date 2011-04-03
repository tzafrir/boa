#include <string.h>

int main() {
  char buf[] = "A string";
  char *p = buf;
  int len = strlen(p);
  p[20] = '\0';
}
