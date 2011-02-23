#include "string.h"

#define STR "A long long long string ... "

int main() {
  char safe1[10], safe2[10], unsafe1[10], unsafe2[10], unsafe3[10];
  
  memcpy(safe1, STR, 10);
  memcpy(safe1, safe2, 10);
  memcpy(unsafe1, STR, 11);
  memcpy(unsafe2, unsafe3, 11);
}
