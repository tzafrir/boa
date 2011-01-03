#include <string.h>

int main() {
  int i;
  char str[6];
  str[0] = 'a';
  str[1] = 'b';
  str[2] = '\0';
  for (i = 0; i < strlen(str); i++) {
    str[i] = '9'; 
  }
  return 0;
}
