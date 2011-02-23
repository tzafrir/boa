#include <stdio.h>

int main() {
  char cString[] = "I am safe!";
  char overflow[8192];
  puts(cString);
  gets(overflow);
  return 0;
}
