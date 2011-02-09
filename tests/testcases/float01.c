int main() {
  double f1 = 10.0;
  float f2 = 5; // Conversion
  char buf1[10], buf2[10];
  
  buf1[(int)f1] = 'a';
  buf2[(int)f2] = 'b';
  
  return 0; 
}
