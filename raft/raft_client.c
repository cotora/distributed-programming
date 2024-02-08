#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <margo.h>
#include <mercury_proc_string.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <stdbool.h>
#include "types.h"

static hg_return_t
call_Submit(const char *addr,int32_t in,int32_t *res);

//margoに関する情報を保持するための構造体
struct env {
    margo_instance_id mid;
    char *addr_str;
    hg_id_t Submit_rpc;
} env;

int
main(int argc,char *argv[]){

    //必要な変数の宣言
    char addr_str[PATH_MAX];
    size_t addr_str_size = sizeof(addr_str);
    hg_addr_t my_address;

    //margoインスタンスの初期化
    margo_instance_id mid = margo_init("tcp", MARGO_CLIENT_MODE, 1,10);
    if (mid == MARGO_INSTANCE_NULL)
            fprintf(stderr, "margo_init failed, abort\n"), exit(1);

    //サーバーアドレスを取得して表示する
    margo_addr_self(mid, &my_address);
    margo_addr_to_string(mid, addr_str, &addr_str_size, my_address);
    margo_addr_free(mid, my_address);
    printf("Server running at address %s\n", addr_str);
    env.addr_str=addr_str;

    //RPCの登録
    env.mid = mid;
    env.Submit_rpc=MARGO_REGISTER(mid, "Submit", int32_t,int32_t,
            NULL);

    printf("set completed\n");

    while(1){
        char serv_addr[100];
        int command;
        scanf("%s %d",serv_addr,&command);
        int32_t res;
        call_Submit(serv_addr,command,&res);
        printf("res : %d\n",res);
    }

    margo_wait_for_finalize(mid);

    return (0);
}

//Submit RPCを呼び出す関数
static hg_return_t
call_Submit(const char *addr,int32_t in,int32_t *res){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;
    int32_t out;

    printf("call Submit RPC\n");

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);

    ret=margo_create(env.mid,serv_addr,env.Submit_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    //RPCを送信
    ret=margo_forward(h,&in);
    if(ret!=HG_SUCCESS)
        goto err;

    //RPCの応答結果を取得する
    ret=margo_get_output(h,&out);
    if(ret!=HG_SUCCESS)
        goto err;

    *res=out;

    margo_free_output(h,&out);

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);
}