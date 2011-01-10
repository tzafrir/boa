void foo(int i) {
  char buf1[10];
  buf1[i] = 'a';
}

void bar(int i) {
  char buf2[10];
  buf2[i] = 'a';
}

int main() {
  foo(0);
  foo(1);
  foo(2);
  bar(10);
  return 0;
}
