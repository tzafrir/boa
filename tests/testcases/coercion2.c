int main() {
  char buf1[10], buf2[10];
  int i1 = 10, i2 = 9;
  unsigned u1 = i1, u2 = i2;
  buf1[u1] = buf2[u2] = 'a';
}
