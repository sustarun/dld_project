#include <string.h>
#include <stdlib.h>
#include <stdio.h>
char single_bit2hex(char inp[4]){
	char res;
	if (strncmp(inp,"0000",4)==0){
		res = '0';
		return res;
	}
	else if(strncmp(inp,"0001",4)==0){
		res = '1';
		return res;
	}
	else if(strncmp(inp,"0010",4)==0){
		res = '2';
		return res;
	}
	else if(strncmp(inp,"0011",4)==0){
		res = '3';
		return res;
	}
	else if(strncmp(inp,"0100",4)==0){
		res = '4';
		return res;
	}
	else if(strncmp(inp,"0101",4)==0){
		res = '5';
		return res;
	}
	else if(strncmp(inp,"0110",4)==0){
		res = '6';
		return res;
	}
	else if(strncmp(inp,"0111",4)==0){
		res = '7';
		return res;
	}
	else if(strncmp(inp,"1000",4)==0){
		 res = '8';
		return res;
	}
	else if(strncmp(inp,"1001",4)==0){
		res = '9';
		return res;
	}
	else if(strncmp(inp,"1010",4)==0){
		res = 'A';
		return res;
	}
	else if(strncmp(inp,"1011",4)==0){
		res = 'B';
		return res;
	}
	else if(strncmp(inp,"1100",4)==0){
		res = 'C';
		return res;
	}
	else if(strncmp(inp,"1101",4)==0){
		res = 'D';
		return res;
	}
	else if(strncmp(inp,"1110",4)==0){
		res = 'E';
		return res;
	}
	else if(strncmp(inp,"1111",4)==0){
		res = 'F';
		return res;
	}
	else {
		res = 'W';
		return res;
	}
}

char* bit_to_hex(char* inp){
	int len=strlen(inp);
	int len1 = len/4;
	char* str=malloc(len1+1*sizeof(char));
	// str="";
	memset(str,0,len1+1);
	for (int i = 0; i < len; i=i+4)
	{
		char substr[4];
		strncpy(substr,inp+i,4);
		// printf("%s\n",substr);
		// substr[5]='\0';
		// printf("%s\n",substr );
		// printf("%d\n",substr=="1011" );
		char c=single_bit2hex(substr);
		// printf("%c\n",c );
		char new[2];
		new[0]= c;
		new[1]='\0';
		// printf("new is %s\n",new);
		strcat(str,new);
		// printf("%s\n", str);
	}
	str[len1+1]='\0';
	return str;
}

// int main(){
// 	printf("%s\n", bit_to_hex("1101101111011011110110111101101111011011110110111101101111011011	"));
// }
