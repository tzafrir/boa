#include "string.h"

#define MYSTR "mystring"
#define SHORT "s"


int main() {
  char buf1[8], buf2[10], buf3[8], buf4[10], buf5[8];
  
  strcpy(buf1, MYSTR);
  strcpy(buf2, MYSTR);
  strcpy(buf4, SHORT);
  strcpy(buf3, buf4);
  strcpy(buf5, buf2);

}
