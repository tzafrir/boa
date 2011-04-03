// BOA-TEST   1
// fixed buffer, write after the end of buffer
int main() {
  int i = 10;
  char buf[10];
  buf[i] = 'a';
  return 0;
}

