#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
int bitXor(int x, int y) 
{
  
  
   int a = x & y;
   int b = ~x & ~y;
   int z = ~a & ~b;
    return z;
}

 
char* decryptor(char C[33], char K[33]) {
   /* my first program in C */
   // int x;
  	// char P[32];
  	int P[32];
 //  	char K[32];
 //   scanf("%s", P);
	// scanf("%s", K);
  for (int i = 0; i < 32; ++i)
  {
    /* code */P[i]=(int)C[i]-'0';
  }
	
// for (int j = 0; j < 32; ++j)
//     {
//       printf("%d",C[j]);
//     }
//     printf("\n");
   int N1=0;
	for(int i = 0; i < 32;i++){
  		if(K[i] == '0'){
  			N1++;
  		}
  	}
  	int T[4];
  	int temp_bool[4];
  	for (int i = 0; i < 4; ++i)
    {
      temp_bool[i]=0;
    }

  	for(int i = 0; i< 32 ; i = i + 4){
  		temp_bool[0] = bitXor(temp_bool[0],(int)K[i]-48);
  		temp_bool[1] = bitXor(temp_bool[1],(int)K[i+1]-48);
  		temp_bool[2] = bitXor(temp_bool[2],(int)K[i+2]-48);
  		temp_bool[3] = bitXor(temp_bool[3],(int)K[i+3]-48);
  	}

  	// T[0] = temp_bool[0];
  	// T[1] = temp_bool[1];
  	// T[2] = temp_bool[2];
  	// T[3] = temp_bool[3];
  	// string T_string = "0000";

  	T[0] = temp_bool[0];
  	T[1] = temp_bool[1];
  	T[2] = temp_bool[2];
  	T[3] = temp_bool[3];

    // for (int j = 0; j < 4; ++j)
    // {
    //   printf("%d",T[j] );
    // }

    // printf("\n");
    int carry=0;
    int adder[4]={1,1,1,1};
    for (int j = 3 ; j >= 0 ; j--)
    {
        int firstBit = T[j];
        int secondBit = adder[j];
 
        // boolean expression for sum of 3 bits
        int sum = bitXor(bitXor(firstBit,secondBit),carry);
 
        T[j] = sum;
 
        // boolean expression for 3-bit addition
        carry = (firstBit & secondBit) | (secondBit & carry) | (firstBit & carry);
    }
    int T_temp[32];
  	for(int i=0; i < N1; i++){


  		// string T_String_temp = "":
  		// strcat(T_String_temp,T);
  		// strcat(T_String_temp,T_String_temp);
  		// strcat(T_String_temp,T_String_temp);
  		// strcat(T_String_temp,T_String_temp);

      for (int j = 0; j < 32; j=j+4)
      {
        T_temp[j]=T[0];
        T_temp[j+1]=T[1];
        T_temp[j+2]=T[2];
        T_temp[j+3]=T[3];
        

      }

      // for (int j = 0; j < 32; ++j)
      // {
        
      //   printf("%d",T_temp[j] );

      // }
      // printf("\n");

    // for (int j = 0; j < 32; ++j)
    // {
    //   printf("%d",T_temp[j] );
    // }

    // printf("\n");
// for (int j = 0; j < 32; ++j)
//     {
//       printf("%d",T_temp[j]);
//     }
//     printf("\n");

  		 int carry=0;
      for (int j = 31 ; j >= 0 ; j--)
    {
        int firstBit = P[j];
        int secondBit = T_temp[j];
 
        // boolean expression for sum of 3 bits
        
 
        P[j] = bitXor(firstBit,secondBit);
 
        // boolean expression for 3-bit addition
        
    }


     carry=0;
    int adder[4]={1,1,1,1};
    for (int j = 3 ; j >= 0 ; j--)
    {
        int firstBit = T[j];
        int secondBit = adder[j];
 
        // boolean expression for sum of 3 bits
        int sum = bitXor(bitXor(firstBit,secondBit),carry);
 
        T[j] = sum;
 
        // boolean expression for 3-bit addition
        carry = (firstBit & secondBit) | (secondBit & carry) | (firstBit & carry);
    }

    //  for (int j = 0; j < 4; ++j)
    // {
    //   printf("%d",T[j]);
    // }
    // printf("\n"); 
	// T = addBitStrings(T, "1");

    //  for (int j = 0; j < 32; ++j)
    // {
    //   printf("%d",C[j]);
    // }
    // printf("\n");


  	}

    char* Pnew= malloc(33*sizeof(char));
    memset(Pnew, 0, 33);
    for (int j = 0; j < 32; ++j)
    {
      /* code */Pnew[j]=P[j]+'0';
    }

    return Pnew;
    
    
}

// int main(){
//   printf("%s\n", decryptor("11111111111111111111111111111111","11100000000000000000001100000000"));
// }