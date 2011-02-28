#include <string.h>

int main() {
  char safe1[10], safe2[10], unsafe1[10], unsafe2[10], unsafe3[2], unsafe4[2], unsafe5[10];
  memmove(safe1, safe2, 5);
  memmove(safe1, &unsafe1[5], 8);
  memmove(&unsafe2[5], safe2, 8);
  memmove(unsafe3, unsafe4, 6);
  char* c = memmove(safe1, &unsafe5[6], 2);
  c[4] = 'a';
  return 0;
}
