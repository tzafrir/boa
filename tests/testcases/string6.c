#include "string.h"

#define LONGSTR "This is a long string"

int main() {
  char buf1[10], buf2[10];
  strncpy(buf1, LONGSTR, 100);
  strncpy(buf2, LONGSTR, 10);
}
