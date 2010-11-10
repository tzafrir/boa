// BOA-TEST   1
// fixed buffer, write after the end of buffer
int main() {
	char buf[10];
	buf[10] = 'a';
	return 0;
}

