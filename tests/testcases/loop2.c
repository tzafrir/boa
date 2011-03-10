int main() {
  char safe[10], unsafe[10];
  for (char *p = unsafe; ; p++) {
    *p = 'a';
  }
  return 0;
}
