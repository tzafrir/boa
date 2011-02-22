#include "unistd.h"

int main() {
  int safe[2], unsafe[1];
  pipe(safe);
  pipe(unsafe);
}
