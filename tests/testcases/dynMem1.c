// BOA-TEST   0
// simple malloc
#include <stdlib.h>

int main() {
	char *p;
	p = (char*)malloc(4);

	p[3] = '!';
	return 0;
}

