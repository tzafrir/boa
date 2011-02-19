int main() {
  int i = 5, j = 7, c = 13;

  // Ternary operator with an expression is compiled into a phi node.
  int k = i < 6 ? c-1 : 14;
  int l = j < 6 ? 12 : c+1;
  char buf1[13], buf2[13], buf3[13];
  buf1[k] = 't';
  buf2[l] = 'z';
  int m = 5;
  int n = m < 6 ? c-5 : c-6;
  buf3[n] = 'a';
}
