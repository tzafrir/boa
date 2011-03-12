char global1[10], global2[10], global3[10], global4[10];
int globInt1 = 15, globInt2 = 9;

int main() {
  int i = 9, j = -3;
  global1[globInt2] = 'g';
  global2[globInt1] = 'b';
  global3[i] = 'g';
  global3[j] = 'b';
  global4[globInt1] = 'b';
  global4[globInt2] = 'g';
}
