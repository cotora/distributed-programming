#include<stdio.h>
#include<stdlib.h>

int main(void){
    for(int i=0;i<10;i++){
        printf("%lf\n",0.15+(rand()*(0.15)/(RAND_MAX)));
    }
    
}