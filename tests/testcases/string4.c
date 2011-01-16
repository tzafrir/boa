#include "string.h"

int main() {
  char buf1[10], buf2[10];
  buf1[strlen("hello")] = 'a'; //OK
  buf2[strlen("This is a long string")] = 'a'; //overrun
}
