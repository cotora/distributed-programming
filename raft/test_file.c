#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct test{
    int a;
    int *b;
};

int main(void){
    struct test t1;
    t1.a=2;
    t1.b=malloc(sizeof(int)*3);
    for(int i=0;i<3;i++){
        t1.b[i]=i;
    }

    struct test t2;

    FILE *fp=fopen("test.dat","w+b");
    if(fp!=NULL){
        fwrite(&t1,sizeof(struct test),1,fp);
        fread(&t2,sizeof(struct test),1,fp);
        fclose(fp);
    }
    else{
        printf("error\n");
    }

    printf("%d\n",t2.a);
    for(int i=0;i<3;i++){
        printf("%d ",t2.b[i]);
    }
    printf("\n");

    free(t1.b);
}