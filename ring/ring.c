#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <margo.h>
#include <mercury.h>
#include <mercury_macros.h>
#include <mercury_proc_string.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <stdbool.h>


typedef struct{
    int32_t n;
    hg_string_t *host;
    int32_t bn;
    hg_string_t *bhost;
}  node_list_t;

static void join(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(join)

static void set_next(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(set_next)

static void set_prev(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(set_prev)

static void list(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(list);

static void election(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(election);

static void coordinator(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(coordinator);

static hg_return_t call_join(const char *addr,hg_string_t saddr,char *out_addr);
static hg_return_t call_set_next(const char *addr,hg_string_t saddr);
static hg_return_t call_set_prev(const char *addr,hg_string_t saddr);
static hg_return_t call_list(const char *addr,node_list_t out);
static hg_return_t call_election(const char *addr,node_list_t out);
static hg_return_t call_coordinator(const char *addr,node_list_t out);


struct env {
        margo_instance_id mid;
        hg_id_t join_rpc, set_next_rpc, set_prev_rpc,list_rpc,election_rpc,coordinator_rpc;
} env;

static inline hg_return_t
hg_proc_node_list_t(hg_proc_t proc, void *data)
{
        node_list_t *l = data;
        hg_return_t ret;
        int i;

        ret = hg_proc_int32_t(proc, &l->n);
        if (ret != HG_SUCCESS)
                return (ret);
        if (hg_proc_get_op(proc) == HG_DECODE)
                l->host = malloc(sizeof(hg_string_t) * l->n);
        assert(l->host);
        for (i = 0; i < l->n; ++i) {
                ret = hg_proc_hg_string_t(proc, &l->host[i]);
                if (ret != HG_SUCCESS)
                        return (ret);
        }
        if (hg_proc_get_op(proc) == HG_FREE)
                free(l->host);
        ret = hg_proc_int32_t(proc,&l->bn);
        if(ret != HG_SUCCESS)
                return (ret);
        if(hg_proc_get_op(proc) == HG_DECODE)
                l->bhost= malloc(sizeof(hg_string_t) * l->bn);
        for(i=0; i < l->bn; ++i) {
                ret = hg_proc_hg_string_t(proc,&l->bhost[i]);
                if(ret != HG_SUCCESS)
                        return (ret);
        }
        if(hg_proc_get_op(proc) == HG_FREE)
                free(l->bhost);
        return (ret);
}


typedef struct{
    char self[PATH_MAX];
    char next[PATH_MAX];
    char prev[PATH_MAX];
    int id;
    int list_flag;
    bool is_coordinator;
    int bn;
    char** bhost;
} node;

node n;

void set_host(int hn,char* ht[]){
    for(int i=0;i<n.bn;i++){
        free(n.bhost[i]);
    }
    free(n.bhost);
    n.bhost=malloc(sizeof(char*)*hn);
    for(int i=0;i<hn;i++){
        n.bhost[i]=strdup(ht[i]);
    }
    n.bn=hn;
}

int set_more_next_node(){
    if(n.bn==0){
        return 0;
    }
    int my_ind=-1;
    for(int i=0;i<n.bn;i++){
        if(strcmp(n.bhost[i],n.self)==0){
            my_ind=i;
        }
    }
    snprintf(n.next,strlen(n.bhost[(my_ind+2)%n.bn])+1,"%s",n.bhost[(my_ind+2)%n.bn]);
    call_set_prev(n.bhost[(my_ind+2)%n.bn],n.self);
    return 1;
}

int hash(char* s){
    int res=0;
    for(int i=0;i<strlen(s);i++){
        res+=s[i];
    }
    return res;
}

void ring_init(margo_instance_id mid,char* addr_str){
    n.list_flag=0;
    /*
    int not_num=0;
    while(!isdigit(addr_str[not_num])){
        not_num++;
    }
    n.id=atoi(addr_str+not_num);
    */
    n.id=hash(addr_str);
    snprintf(n.self,sizeof(n.self),"%s",addr_str);
    snprintf(n.prev,sizeof(n.prev),"%s",addr_str);
    snprintf(n.next,sizeof(n.next),"%s",addr_str);
    n.is_coordinator=false;
    n.bn=0;
    n.bhost=NULL;
}

void
join_ring(const char *server)
{
    snprintf(n.next,strlen(server)+1,"%s",server);
    call_join(server,n.self,n.prev);
    printf("self:%s next:%s prev:%s\n",n.self,n.next,n.prev);
}

void
list_ring()
{
    node_list_t initialList;
    initialList.n=1;
    initialList.host=malloc(sizeof(hg_string_t));
    initialList.host[0]=malloc(PATH_MAX);
    snprintf(initialList.host[0],sizeof(n.self),"%s",n.self);
    initialList.bn=n.bn;
    initialList.bhost=malloc(sizeof(hg_string_t)*initialList.bn);
    for(int i=0;i<initialList.bn;i++){
        initialList.bhost[i]=strdup(n.bhost[i]);
    }
    n.list_flag=1;
    if(call_list(n.next,initialList)!=HG_SUCCESS){
        if(set_more_next_node()){
            printf("The next has been reconnected.\n");
            call_list(n.next,initialList);
        }
        else{
            printf("set_more_next_node error.\n");
        }
    }
    free(initialList.host[0]);
    free(initialList.host);
    for(int i=0;i<initialList.bn;i++){
        free(initialList.bhost[i]);
    }
    free(initialList.bhost);
}

void
election_ring()
{
    node_list_t initialList;
    initialList.n=1;
    
    initialList.host=malloc(sizeof(hg_string_t));
    initialList.host[0]=malloc(PATH_MAX);
    snprintf(initialList.host[0],sizeof(n.self),"%s",n.self);
    if(call_election(n.next,initialList)!=HG_SUCCESS){
        if(set_more_next_node()){
            printf("The next has been reconnected.\n");
            call_election(n.next,initialList);
        }
        else{
            printf("set_more_next_node error.\n");
        }
    }
    free(initialList.host[0]);
    free(initialList.host);
    
}

void leave(){
    if(strcmp(n.self,n.next)==0)return;
    call_set_next(n.prev,n.next);
    call_set_prev(n.next,n.prev);
}

void hearbeat(){
    while(1){
        margo_thread_sleep(env.mid,3000);
        list_ring();
    }
}

void *
handle_sig(void *arg)
{
    sigset_t *a=arg;
    int sig;

    sigwait(a,&sig);
    leave();
    exit(1);
}

void ring_set_next(char* next){
    snprintf(n.next,sizeof(n.next),"%s",next);
}

void ring_set_prev(char *prev){
    snprintf(n.prev,sizeof(n.prev),"%s",prev);
}

int
main(int argc, char *argv[])
{
        char addr_str[PATH_MAX];
        size_t addr_str_size = sizeof(addr_str);
        hg_addr_t my_address;

        pthread_t t;
        static sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset,SIGINT);
        sigaddset(&sigset,SIGTERM);
        pthread_sigmask(SIG_BLOCK,&sigset,NULL);
        pthread_create(&t,NULL,handle_sig,&sigset);
        pthread_detach(t);

        margo_instance_id mid = margo_init("tcp", MARGO_SERVER_MODE, 1, 10);
        if (mid == MARGO_INSTANCE_NULL)
                fprintf(stderr, "margo_init failed, abort\n"), exit(1);

        margo_addr_self(mid, &my_address);
        margo_addr_to_string(mid, addr_str, &addr_str_size, my_address);
        margo_addr_free(mid, my_address);
        printf("Server running at address %s\n", addr_str);

        env.mid = mid;
        env.join_rpc = MARGO_REGISTER(mid, "join", hg_string_t, hg_string_t,
                join);
        env.set_next_rpc = MARGO_REGISTER(mid, "set_next", hg_string_t, void,
                set_next);
        margo_registered_disable_response(mid, env.set_next_rpc, HG_TRUE);
        env.set_prev_rpc = MARGO_REGISTER(mid, "set_prev", hg_string_t, void,
                set_prev);
        margo_registered_disable_response(mid, env.set_prev_rpc, HG_TRUE);
        env.list_rpc = MARGO_REGISTER(mid,"list",node_list_t,int32_t,list);
        //margo_registered_disable_response(mid, env.list_rpc, HG_TRUE);
        env.election_rpc = MARGO_REGISTER(mid, "election", node_list_t, int32_t,election);
        //margo_registered_disable_response(mid, env.election_rpc, HG_TRUE);
        env.coordinator_rpc = MARGO_REGISTER(mid, "coordinator", node_list_t, int32_t,coordinator);
        //margo_registered_disable_response(mid, env.coordinator_rpc, HG_TRUE);
        ring_init(mid, addr_str);
        if (argc > 1)
                join_ring(argv[1]);

        if(argc>2 && strcmp(argv[2],"list")==0){
            list_ring();
        }

        time_t prev_time=time(NULL);
        while(1){
            if(!n.is_coordinator){
                time_t cur_time=time(NULL);
                if(abs(cur_time-prev_time)>5){
                    printf("start election.\n");
                    election_ring();
                    while(!n.list_flag){
                    }
                }
                if(n.list_flag){
                    prev_time=cur_time;
                    printf("list detected.\n");
                    n.list_flag=0;
                }
                //printf("%ld %ld\n",prev_time,cur_time);
            }
        }
        margo_wait_for_finalize(mid);
        
        return (0);
}

static hg_return_t
call_set_next(const char *addr,hg_string_t saddr){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);

    ret=margo_create(env.mid,serv_addr,env.set_next_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    ret=margo_forward(h,&saddr);
    if(ret!=HG_SUCCESS)
        goto err;


err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

static hg_return_t
call_set_prev(const char *addr,hg_string_t saddr){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);

    ret=margo_create(env.mid,serv_addr,env.set_prev_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    ret=margo_forward(h,&saddr);
    if(ret!=HG_SUCCESS)
        goto err;

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

static hg_return_t
call_join(const char *addr,hg_string_t saddr,char *out_addr){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_string_t out;
    hg_addr_t serv_addr;

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);
    ret=margo_create(env.mid,serv_addr,env.join_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    ret=margo_forward(h,&saddr);
    if(ret!=HG_SUCCESS)
        goto err;

    ret=margo_get_output(h,&out);
    if(ret!=HG_SUCCESS)
        goto err;

    snprintf(out_addr,PATH_MAX,"%s",out);

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

static hg_return_t
call_list(const char *addr,node_list_t out){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;
    int32_t res;

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);
    if(ret!=HG_SUCCESS)return (ret);
    ret=margo_create(env.mid,serv_addr,env.list_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    ret=margo_forward_timed(h,&out,2000);
    if(ret!=HG_SUCCESS)
        goto err;
    
    ret=margo_get_output(h,&res);
    if(ret!=HG_SUCCESS)
        goto err;

    margo_free_output(h,&res);

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

static hg_return_t
call_election(const char *addr,node_list_t out){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;
    int32_t res;

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);
    if(ret!=HG_SUCCESS)return (ret);
    ret=margo_create(env.mid,serv_addr,env.election_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    ret=margo_forward_timed(h,&out,2000);
    if(ret!=HG_SUCCESS)
        goto err;

    ret=margo_get_output(h,&res);
    if(ret!=HG_SUCCESS)
        goto err;

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

static hg_return_t
call_coordinator(const char *addr,node_list_t out){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;
    int32_t res;

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);
    if(ret!=HG_SUCCESS)return (ret);
    ret=margo_create(env.mid,serv_addr,env.coordinator_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    ret=margo_forward_timed(h,&out,2000);
    if(ret!=HG_SUCCESS)
        goto err;

    ret=margo_get_output(h,&res);
    if(ret!=HG_SUCCESS)
        goto err;

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

static void
join(hg_handle_t h)
{
        hg_return_t ret;
        hg_string_t in;
        hg_string_t out;

        ret=margo_get_input(h,&in);
        assert(ret==HG_SUCCESS);

        call_set_next(n.prev,in);

        out=malloc(sizeof(char)*PATH_MAX);
        snprintf(out,strlen(n.prev)+1,"%s",n.prev);
        snprintf(n.prev,strlen(in)+1,"%s",in);
        ret=margo_free_input(h,&in);
        assert(ret==HG_SUCCESS);

        ret=margo_respond(h,&out);
        assert(ret==HG_SUCCESS);

        ret=margo_destroy(h);
        assert(ret==HG_SUCCESS);

        free(out);
}
DEFINE_MARGO_RPC_HANDLER(join)

static void
set_next(hg_handle_t h)
{
        hg_return_t ret;
        char *in;

        ret = margo_get_input(h, &in);
        assert(ret == HG_SUCCESS);

        ring_set_next(in);

        ret = margo_free_input(h, &in);
        assert(ret == HG_SUCCESS);

        ret = margo_destroy(h);
        assert(ret == HG_SUCCESS);

        printf("self:%s next:%s prev:%s\n",n.self,n.next,n.prev);

}
DEFINE_MARGO_RPC_HANDLER(set_next)

static void
set_prev(hg_handle_t h)
{
        hg_return_t ret;
        char *in;

        ret = margo_get_input(h, &in);
        assert(ret == HG_SUCCESS);

        ring_set_prev(in);

        ret = margo_free_input(h, &in);
        assert(ret == HG_SUCCESS);

        ret = margo_destroy(h);
        assert(ret == HG_SUCCESS);

        printf("self:%s next:%s prev:%s\n",n.self,n.next,n.prev);
}
DEFINE_MARGO_RPC_HANDLER(set_prev)

static void
list(hg_handle_t h)
{
    hg_return_t ret;
    node_list_t in;
    int32_t res=1;

    ret=margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&res);
    assert(ret==HG_SUCCESS);

    n.list_flag=1;

    if(strcmp(in.host[0],n.self)==0){
        printf("[ ");
        for(int i=0;i<in.n;i++){
            if(i==in.n-1){
                printf("%s ]",in.host[i]);
            }
            else{
                printf("%s, ",in.host[i]);
            }
        }
        printf("\n");
        set_host(in.n,in.host);
        //printf("We've come full circle.\n");
    }
    else{
        set_host(in.bn,in.bhost);

        node_list_t out;
        out.n=in.n+1;
        out.host=malloc(sizeof(hg_string_t)*out.n);
        //printf("ok1\n");
        for(int i=0;i<in.n;i++){
            out.host[i]=(char *)malloc(sizeof(char)*PATH_MAX);
            //printf("%s %s\n",out.host[i],in.host[i]);
            snprintf(out.host[i],PATH_MAX,"%s",in.host[i]);
        }
        out.host[in.n]=(char *)malloc(sizeof(char)*PATH_MAX);
        snprintf(out.host[in.n],strlen(n.self)+1,"%s",n.self);
        //printf(n)

        out.bn=in.bn;
        out.bhost=in.bhost;
    
        if(call_list(n.next,out)!=HG_SUCCESS){
            if(set_more_next_node()){
                printf("The next has been reconnected.\n");
                call_list(n.next,out);
            }
            else{
                printf("set_more_next_node error.\n");
            }
        }
    

        for(int i=0;i<out.n;i++){
            free(out.host[i]);
        }
        free(out.host);

    }

    ret=margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_destroy(h);
    assert(ret==HG_SUCCESS);

}
DEFINE_MARGO_RPC_HANDLER(list)

static void
election(hg_handle_t h)
{
    hg_return_t ret;
    node_list_t in;
    int32_t res=1;

    ret=margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&res);
    assert(ret==HG_SUCCESS);

    if(strcmp(in.host[0],n.self)==0){
        for(int i=0;i<in.n;i++){
            printf("%s ",in.host[i]);
        }
        printf("\n");
        printf("We've come full circle.\n");
        set_host(in.n,in.host);
        call_coordinator(n.self,in);
    }
    else{
        node_list_t out;
        out.n=in.n+1;
        out.host=malloc(sizeof(hg_string_t)*out.n);
        //printf("ok1\n");
        for(int i=0;i<in.n;i++){
            out.host[i]=(char *)malloc(sizeof(char)*PATH_MAX);
            //printf("%s %s\n",out.host[i],in.host[i]);
            snprintf(out.host[i],PATH_MAX,"%s",in.host[i]);
        }
        out.host[in.n]=(char *)malloc(sizeof(char)*PATH_MAX);
        snprintf(out.host[in.n],strlen(n.self)+1,"%s",n.self);
        //printf(n)

        if(call_election(n.next,out)!=HG_SUCCESS){
            if(set_more_next_node()){
                printf("The next has been reconnected.\n");
                call_election(n.next,out);
            }
            else{
                printf("set_more_next_node error.\n");
            }
        }
        for(int i=0;i<out.n;i++){
            free(out.host[i]);
        }
        free(out.host);
    }

    ret=margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_destroy(h);
    assert(ret==HG_SUCCESS);

}
DEFINE_MARGO_RPC_HANDLER(election)

static void
coordinator(hg_handle_t h)
{
    hg_return_t ret;
    node_list_t in;
    int32_t res;

    ret=margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&res);
    assert(ret==HG_SUCCESS);

    int max_id=-1;
    for(int i=0;i<in.n;i++){
        /*
        int not_num=0;
        while(!isdigit(in.host[i][not_num])){
            not_num++;
        }
        int tmp_id=atoi(in.host[i]+not_num);
        */
        int tmp_id=hash(in.host[i]);
        if(max_id<tmp_id){
            max_id=tmp_id;
        }
    }

    set_host(in.n,in.host);

    //printf("%d %d\n",max_id,n.id);
    if(max_id==n.id){
        n.is_coordinator=true;
        printf("I am coordinator. : %d\n",n.id);
        hearbeat();
    }
    else{
        if(call_coordinator(n.next,in)!=HG_SUCCESS){
            if(set_more_next_node()){
                printf("The next has been reconnected.\n");
                call_coordinator(n.next,in);
            }
            else{
                printf("set_more_next_node error.\n");
            }
        }
    }

    ret=margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_destroy(h);
    assert(ret==HG_SUCCESS);

}
DEFINE_MARGO_RPC_HANDLER(coordinator)