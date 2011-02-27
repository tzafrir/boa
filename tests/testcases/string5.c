#include "string.h"

int main() {
  char buf1[6], buf2[21];
  char *p1 = "hello", *p2 = "This is a long string";
  buf1[strlen(p1)] = 'a'; //OK
  buf2[strlen(p2)] = 'a'; //overrun
}
