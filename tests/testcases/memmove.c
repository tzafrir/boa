#include <string.h>

int main() {
  char safe1[10], safe2[10], unsafe1[10], unsafe2[10], unsafe3[2], unsafe4[2], unsafe5[10];
  memmove(safe2, safe1, 10);
  memmove(&unsafe1[5], safe1, 10);
  memmove(safe2, &unsafe2[5], 10);
  memmove(unsafe4, unsafe3, 6);
  char* c = memmove(unsafe5, safe1, 2);
  c[20] = 'a';
  return 0;
}
