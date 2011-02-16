int main() {
  int i = 5, j = 7, c = 13;

  // Ternary operator with an expression is compiled into a phi node.
  int k = i < 6 ? c-1 : 14;
  int l = j < 6 ? 12 : c+1;
  char buf1[13], buf2[13];
  buf1[i] = 't';
  buf2[j] = 'z';
}
