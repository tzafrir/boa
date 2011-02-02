int main() {
  char buf1[10], buf2[10], buf3[10], buf4[10], buf5[10];
  int i = 3, j = 5, k, l = 0;
  char c = 'a';
  buf1[i] = c;
  buf1[i + j] = c;
  buf1[2 * i - j] = c;
  buf1[4 + j] = c;
  buf2[i - j] = c; // write before the begining of buffer
//  buf3[k] = c; // k uninitialized
//  k = 1;
  l = j + 5;
  buf4[l] = c; // l == 10
}

