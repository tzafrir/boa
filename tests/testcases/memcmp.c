#include <string.h>

int main() {
  char safe1[20], safe2[20], unsafe1[10], unsafe2[10], unsafe3[10], unsafe4[10], unsafe5[8192];
  int i;
  i = memcmp(safe1, safe2, 19);
  i = memcmp(unsafe1, safe2, 19);
  i = memcmp(safe1, unsafe2, 19);
  i = memcmp(unsafe3, unsafe4, 19);
  int j = memcmp(safe1, safe2, 1);  // No guarantees are given on the return value, except that
                                    // it is positive or negative or 0.
  unsafe5[i] = 'a';
  return 0;
}
