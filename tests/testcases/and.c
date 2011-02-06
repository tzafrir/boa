int main() {
  char buf1[10], buf2[12];
  int i = 10, j = 11;
  int k = i & j;
  buf1[k] = buf2[k] = 'a';
}
