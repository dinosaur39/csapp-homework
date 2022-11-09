#include<stdio.h>
#include<string.h>
int main()
{
	char print[200];
	int i = 0;
	for(int j = 0; j < 40; j++) 
	{
		strcpy(print + i, "00 ");
		i += 3;
	}
	strcpy(print + i, "c0 17 40");
	printf("%s 0a ", print);
	return 0;
}
