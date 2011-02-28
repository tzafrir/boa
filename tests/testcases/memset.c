#include <string.h>

int main() {
  char safe[20], unsafe[10];
  memset(safe, '\0', 19);
  memset(unsafe, '\0', 19);
  return 0;
}
