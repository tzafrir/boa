#include <stdio.h>
int main() {
  char* buf1 = malloc(4 * sizeof(char));
  buf1[1] = 'a';

  char* buf2 = malloc(4 * sizeof(char));
  buf2[4] = 'a';

  char* buf3 = malloc(5 * sizeof(char));
  buf3[3] = 'a';
}
