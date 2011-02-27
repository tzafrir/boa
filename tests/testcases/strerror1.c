#include "string.h"

int main() {
  char *safe = strerror(1);
  char *unsafe = strerror(1);
  unsafe[1] = 'a';
}
