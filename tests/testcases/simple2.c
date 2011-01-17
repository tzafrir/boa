// BOA-TEST   0
// fixed buffer, no write access
int hello = 4;

int main() {
	char buf[10];
	char c = buf[17];
	return 15;
}

int myFunc2() {
  return 2;
}
