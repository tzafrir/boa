#include <stdlib.h>
int main() {
  char* buf1 = malloc(4 * sizeof(char));
  buf1[4] = 'a';

  char* buf2 = malloc(5 * sizeof(char));
  buf2[4] = 'a';
}
