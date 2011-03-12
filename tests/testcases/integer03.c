int main() {
  char buf1[10], buf2[10], buf3[10], buf4[10], buf5[10];
  int i=5, j=5, k=5, l=5, m=5;
  for (;;) {
    i++;
    j--;
    k += 1;
    l -= 1;
    buf1[i] = buf2[j] = buf3[k] = buf4[l] = buf5[m];
  }
  return buf1[i];
}
