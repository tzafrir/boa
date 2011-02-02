struct s {
  int i;
  char buf[10];
};

int main() {
  struct s s1, s2;
  s1.buf[1] = s2.buf[11];
}
