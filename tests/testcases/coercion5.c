int main() {
  char buf1[10], buf2[10];
  char c1 = 10, c2 = 9;
  long l1 = c1, l2 = c2;
  buf1[l1] = buf2[l2] = 'a';
}
