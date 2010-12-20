// BOA-TEST   1
// simple malloc
#include <stdlib.h>

int main() {
	char *p;
	p = (char*)malloc(4);

	p[5] = '!';
	return 0;
}

