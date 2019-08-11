#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int decconverter(int num){
      int  decimal_val = 0, base = 1, rem;
while (num > 0)
    {
        rem = num % 10;
        decimal_val = decimal_val + rem * base;
        num = num / 10 ;
        base = base * 2;
    }
    return decimal_val;
}

void write_csv(char str[8],  char* filename, int x , int y)
{
    // int a= str[0]-'0';
    int b= str[1]-'0';
    char dir[3];
    for (int i = 2; i < 5; ++i)
    {
        dir[i-2]=str[i];
    }
    printf("%s\n", dir);
    int c= atoi(dir);
    c= decconverter(c);
    char nextdir[3];
    for (int i = 5; i < 8; ++i)
    {
        nextdir[i-5]=str[i];
    }
    printf("%s\n", nextdir);
    int d= atoi(nextdir);
    d= decconverter(d);
    char xcor=x+'0';
    char ycor=y+'0';
    printf("%c %c\n", xcor,ycor);
    char temp[10];

    sprintf(temp,"%d,%d,%d,%d,%d",x,y,c,b,d);

    printf("%s\n", temp);
    FILE * pFile;
    fpos_t pos1,pos2;
    int line = 0;
    char buf[68];
    // char *p;    
    pFile = fopen (filename,"r+");

    printf("changes are made in this lines:\t");    
    while ( !feof(pFile) ){
        ++line;
        fgetpos ( pFile, &pos1);            

        if( fgets( buf, 68, pFile ) == NULL )   
            break;

        fgetpos ( pFile, &pos2);


        if( buf[0]==xcor && buf[2]==ycor && buf[4]==c+'0'){
            printf("%d, " ,line);
            fsetpos( pFile, &pos1 );    
            fputs(temp, pFile);
        }
        fsetpos( pFile, &pos2);
    }

    fclose (pFile);
}

// int main(){
//     write_csv("11100101", "1.csv", 2 , 2);
// }