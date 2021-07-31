int	write(int, char*, int);

int
main(int a, int b)
{
	int i;
	char c;

	for(i = 'a'; i < 'z'; i = i+1) {
		switch(i) {
		case 'a':
			c = 'a';
			break;
		case 'c':
			c = 'c';
			break;
		case 'b':
			c = 'b';
			break;
		case 'f':
			c = 'f';
			break;
		case 'h':
			c = 'h';
			break;
		case 'o':
			c = 'o';
			break;
		case 'p':
			c = 'p';
			break;
		case 'q':
			c = 'q';
			break;
		case 'y':
			c = 'y';
			break;
		default:
			c = 'd';
		}
		write(1, &c, 1);
	}
	return a;
}
