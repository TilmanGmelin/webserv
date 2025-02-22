#include <stdio.h>

int main(void)
{
	long long counter = 0;

	while (counter < 10000000000000000)
	{
		printf("Counter: %lld\n", counter);
		counter++;
	}
	
	return 0;
}