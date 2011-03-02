#include <string.h>

int main() {
  char unsafe[4] = "abc";
  char safe[] = "Safe string";
  char safeAlias[10] = "123456789";
  char unsafeAlias[4] = "abc";
  char* c = memchr(unsafe, 'b', 10);
  if (c != NULL) return 0;
  c = memchr(safe, 'z', 2);
  char* a1 = memchr(safeAlias, '1', 1);
  a1[3] = '0';
  char* a2 = memchr(unsafeAlias, 'd', 3);
  a2[3] = '0';
  return 0;
}
