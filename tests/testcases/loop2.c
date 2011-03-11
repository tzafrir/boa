int main() {
  char safe[10], unsafe[10];
  char *p;
  for (p = unsafe; ; p++) {
    *p = 'a';
  }
  return 0;
}
