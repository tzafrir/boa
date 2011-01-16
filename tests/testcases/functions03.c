int add(int a, int b) {
  return a+b;
}

int main() {
  char buf1[10], buf2[10];
  int i = add(1,1);
  int j = add(1, 10);
  buf1[i] = buf2[j];
}
