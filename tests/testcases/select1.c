int main() {
  int i = 5, j = 7;

  // Ternary operator with constants compiles into a select node.
  int k = i < 6 ? 12 : 14;
  int l = j < 6 ? 12 : 14;
  int m = j < 6 ? 10 : 11;
  char buf1[13], buf2[13], buf3[13];
  buf1[k] = 't';
  buf2[l] = 'z';
  buf3[m] = 'a';
}
