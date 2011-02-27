 struct s {
    int i;
    char buf[10];
    struct t {
      double x;
      char buf[20];
    } inner;
  };

int main() {
  struct s s1, s2;
  s1.buf[1] = s2.inner.buf[20];
}
