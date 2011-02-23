#include "string.h"

int main() {
  char source[] = "hello";
  char *unsafe = strdup(source);
  char *safe = strdup(source);
  unsafe[10] = 'a'; //overrun
}
