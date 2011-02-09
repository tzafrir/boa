int main() {
  char buf1[10], buf2[20];
  int i = 4, j = 8;
  int k = i | j;
  buf1[k] = buf2[k] = 'a';
}
