int foo(char* buff) {
  buff[10] = 'a';
  return 0;
}

int main() {
  char buf1[10], buf2[11];
  foo(buf1);
  foo(buf2);
  return 0;
}
