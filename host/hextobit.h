#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
int convert(int dec)
{
    if (dec == 0)
    {
        return 0;
    }
    else
    {
        return (dec % 2 + 10 * convert(dec / 2));
    }
}
char* hextobit(char hexdig){
	// len=strlen(hexstr);
	long n = strtol(hexstr, NULL, 16);
	int bits= convert((int)n);
	char* bitstring= malloc(32*sizeof(char));
	sprintf(bitstring, "%d", bits);
	return bitstring;
}
// int main(){
// 	printf("%s\n", hextobit("E"));
// }