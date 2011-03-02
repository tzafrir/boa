#include "string.h"

int main() {
  char *safe = strerror(-127);
  char *unsafe = strerror(-127);
  unsafe[-1] = 'a';
}
