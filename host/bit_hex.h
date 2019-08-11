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

// int intlength(int n){
//    int count=0;
//    while(n != 0)
//     {
//         // n = n/10
//         n /= 10;
//         ++count;
//     }
//     return count;

// }
// int decconverter(int num){
//       int binary_val, decimal_val = 0, base = 1, rem;
// while (num > 0)
//     {
//         rem = num % 10;
//         decimal_val = decimal_val + rem * base;
//         num = num / 10 ;
//         base = base * 2;
//     }
//     return decimal_val;
// }
// const char* converter(int num){
//    int m= convert(num);
//    if (intlength(m) < 3){
      
//       int d1= num/10;
//       int d2= num%10;
//       char str2[2];
//       //  printf("%s\n%s", d1,d2);
//       str2[0]=d1+'0';
//       str2[1]= d2+'0';
//       printf("%s\n", str2);
//       char str1[]="0";
//       char* str= malloc(3*sizeof(char));
//       strcat(str1, str2);
//       printf("%s\n",str1 );
//       str=str1;
//       return str;
//    }
//    else {
//     char* str= malloc(3*sizeof(char));
//     for (int i = 0; i < 3; ++i)
//     {
//       /* code */
//     }
//    }


//  }

char * bitmaker(bool a, bool b, int c, int d){
  int num = 10*10*10*10*10*10*10*(int)a+10*10*10*10*10*10*(int)b+10*10*10*convert(c)+convert(d);
  // printf("num is %d\n", num);
  // int byte1= new/(10*10*10*10);
  // int byte2= new % (10*10*10*10);
  // byte1=decconverter(byte1);
  // byte2=decconverter(byte2);
  char str1[9];
  // itoa(str, num, 10)
  // char str2[2];
  // printf("%d\n", byte1);
  // printf("%d\n", byte2);
  // snprintf(str, sizeof(str), "%X", new);
  sprintf(str1, "%08d", num);
  // printf("str1 is %s\n", str1);

  // printf("%s\n",str1);
  // printf("%s\n",str2);
  // strcat(str1,str2);
  // printf("%s\n",str1);
  char* str= malloc(9*sizeof(char));
  memset(str, 0, 9);
  for (int i = 0; i < 8; ++i)
  {
      str[i]=str1[i];
  }
  // printf("str is %s\n", str);
  return str;

}

// const char * hex_to_bit(char hex_str[]){
//   int num = (int)strtol(hex_str, NULL, 16);
//   sprintf("%X\n", num);
//   decconverter()
// }

// char * hex_to_bit(char hexstr[]){
//   // len=strlen(hexstr);
//   printf("hexstr is %s\n", hexstr);
//   long n = (long)strtol(hexstr, NULL, 16);
//   printf("n is %d\n", n);
//   long bits= convert((int)n);
//   printf("bits are %d\n", bits);
//   char* bitstring= malloc(32*sizeof(char));
//   sprintf(bitstring, "%032d", bits);
//   return bitstring;
// }
char* single_hex2bits(char* inp){
  char* res = malloc(4* sizeof(char));
  if (inp[0] == '0'){
    res = "0000";
    return res;
  }
  else if(inp[0] == '1'){
    res = "0001";
    return res;
  }
  else if(inp[0] == '2'){
    res = "0010";
    return res;
  }
  else if(inp[0] == '3'){
    res = "0011";
    return res;
  }
  else if(inp[0] == '4'){
    res = "0100";
    return res;
  }
  else if(inp[0] == '5'){
    res = "0101";
    return res;
  }
  else if(inp[0] == '6'){
    res = "0110";
    return res;
  }
  else if(inp[0] == '7'){
    res = "0111";
    return res;
  }
  else if(inp[0] == '8'){
    char* res = "1000";
    return res;
  }
  else if(inp[0] == '9'){
    res = "1001";
    return res;
  }
  else if(inp[0] == 'A'){
    res = "1010";
    return res;
  }
  else if(inp[0] == 'B'){
    res = "1011";
    return res;
  }
  else if(inp[0] == 'C'){
    res = "1100";
    return res;
  }
  else if(inp[0] == 'D'){
    res = "1101";
    return res;
  }
  else if(inp[0] == 'E'){
    res = "1110";
    return res;
  }
  else if(inp[0] == 'F'){
    res = "1111";
    return res;
  }
  else {
  	res = "WWWW";
  	return res;
  }
}

char* hex_to_bit(char* inp){
  int len = strlen(inp);
  char* res =  malloc((4*len+1) * sizeof(char));
  memset(res, 0, (4*len+1));
  for(int i = 0; i < 8; i++){
    strcat(res, single_hex2bits(&inp[i]));
  }
  return res;
}

// int main(){
//    printf("%s\n",bitmaker(0,1,5,4));
//    char hex[16] = "A567";
//    int num = (int)strtol(hex, NULL, 16);
//    // sprintf(num,"%X\n", num);
//    sprintf(num1, "%d\n", num);
// }