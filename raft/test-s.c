#include<stdio.h>

int main(void){
    FILE *fp;
    fp=fopen("test.txt","w");
    if(fp!=NULL){
        fprintf(fp,"0");
        fclose(fp);
        for(int i=0;i<10;i++){
            int cur=0;
            fp=fopen("test.txt","r");
            if(fp!=NULL){
                if(fscanf(fp,"%d",&cur)!=1){
                    printf("error\n");
                }
                else{
                    printf("cur : %d\n",cur);
                }
                fclose(fp);
            }
            fp=fopen("test.txt","w");
            if(fp!=NULL){
                cur+=i;
                fprintf(fp,"%d",cur);
                fclose(fp);
            }
            }
        }
        return 0;
    }