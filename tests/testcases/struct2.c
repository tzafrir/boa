typedef struct s {
  int i;
  char buf[10];
} sType;

int main() {
  sType s1, s2;
  sType *p1 = &s1;
  p1->buf[1] = s2.buf[11];
}
