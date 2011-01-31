#include <string.h>
#include <stdlib.h>

int main() {
  char *str = malloc(strlen("a string")+1);
  char ch = str[strlen(str)+1];
  return 0;
}
