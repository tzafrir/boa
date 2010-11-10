#include <iostream>

int main(int argc, char* argv[]) {
  std::cout << "Buffer overruns are possible!" << std::endl;
  return 1; // non zero return value means fail => buffer overruns are possible
}
