#include "stdio.h"
#include "string.h"

int main() {
  char unsafe1[100], unsafe2[10], safe[10];
  scanf("%s", unsafe1);
  strcpy(unsafe2, unsafe1);
  strncpy(safe, unsafe1, 10);
  return 0;
}
