int main() {
  char buf1[10], buf2[10];
  char c1 = 10, c2 = 9;
  short s1 = c1, s2 = c2;
  buf1[s1] = buf2[s2] = 'a';
}
