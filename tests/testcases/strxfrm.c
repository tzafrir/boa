#include "string.h"

int main() {
  char safe[10], unsafe[10];
  strxfrm(safe, "long long long string", 10);
  strxfrm(unsafe, "long long long string", 11);  
}
