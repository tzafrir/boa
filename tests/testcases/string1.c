#include "string.h"

int main() {
  char *str1 = "longer than ten", *str2 = "short";
  char buf1[10], buf2[10];
  strcpy(buf1, str1);
  strcpy(buf2, str2);
}
