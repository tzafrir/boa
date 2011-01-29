// BOA-TEST   1
// simple malloc
#include <stdlib.h>

int main() {
	char *p;
	int i = 4;
	p = (char*)malloc(i);

	p[5] = 'a';
	return 0;
}

