#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
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
#include <fcntl.h>
#include "types.h"

static void getattr(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(getattr)

static void readdir_s(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(readdir_s)

static void open_s(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(open_s)

static void read_s(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(read_s)

static void read_rdma(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(read_rdma);

int main(int argc,char *argv[]){
	margo_instance_id mid;
	char addr_str[PATH_MAX];
	size_t addr_str_size = sizeof(addr_str);
	hg_addr_t my_address;

	mid = margo_init("tcp", MARGO_SERVER_MODE, 0, 0);
	assert(mid);

	margo_addr_self(mid, &my_address);
	margo_addr_to_string(mid, addr_str, &addr_str_size, my_address);
	margo_addr_free(mid, my_address);
	printf("Server running at address %s\n", addr_str);

	MARGO_REGISTER(mid, "getattr", hg_string_t, stat_t, getattr);
    MARGO_REGISTER(mid, "readdir", hg_string_t, dirents_t, readdir_s);
    MARGO_REGISTER(mid, "open", open_in_t, int32_t, open_s);
    MARGO_REGISTER(mid, "read", read_in_t, read_out_t, read_s);
    MARGO_REGISTER(mid, "read_rdma", read_rdma_in_t, read_rdma_out_t, read_rdma);

	margo_wait_for_finalize(mid);

	return (0);
}

static void
getattr(hg_handle_t h){
    hg_return_t ret;
    hg_string_t in;
    stat_t out;

    printf("called getattr\n");

    ret = margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    int ret2;
    struct stat st;

    ret2 = lstat(in,&st);

    if(ret2==-1){
        out.error=-errno;
        out.ino=0;
        out.mode=0;
        out.nlink=0;
        out.uid=0;
        out.gid=0;
        out.size=0;
        out.atime=0;
        out.mtime=0;
        out.ctime=0;
    }
    else{
        out.error=0;
        out.ino=st.st_ino;
        out.mode=st.st_mode;
        out.nlink=st.st_nlink;
        out.uid=st.st_uid;
        out.gid=st.st_gid;
        out.size=st.st_size;
        out.atime=st.st_atime;
        out.mtime=st.st_mtime;
        out.ctime=st.st_ctime;
    }

    ret = margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&out);
    assert(ret==HG_SUCCESS);

    ret = margo_destroy(h);
    assert(ret==HG_SUCCESS);
}
DEFINE_MARGO_RPC_HANDLER(getattr)

static void
readdir_s(hg_handle_t h){
    hg_return_t ret;
    hg_string_t in;
    dirents_t out;

    printf("called readdir\n");

    ret = margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    DIR *dp;
    struct dirent *dent;

    dp=opendir(in);
    if(dp==NULL){
        out.error=-errno;
        out.n=0;
        out.d=NULL;
    }
    else{
        out.error=0;

        int n=0;
        while((dent=readdir(dp))!=NULL){
            n++;
        }

        out.n=n;
        out.d=malloc(sizeof(dirent_t)*n);

        rewinddir(dp);
        int i=0;
        while((dent = readdir(dp))!=NULL){
            out.d[i].ino=dent->d_ino;
            out.d[i].type=dent->d_type;
            out.d[i].name=malloc(strlen(dent->d_name)+1);
            snprintf(out.d[i].name,strlen(dent->d_name)+1,"%s",dent->d_name);
            i++;
        }
        closedir(dp);
    }

    ret = margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&out);
    assert(ret==HG_SUCCESS);

    for(int i=0;i<out.n;i++){
        free(out.d[i].name);
    }
    free(out.d);

    ret = margo_destroy(h);
    assert(ret==HG_SUCCESS);    
}
DEFINE_MARGO_RPC_HANDLER(readdir_s)

static void
open_s(hg_handle_t h){
    hg_return_t ret;
    open_in_t in;
    int32_t out;

    printf("called open\n");

    ret = margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    int ret2;

    ret2 = open(in.path,in.flags);
    if(ret2 == -1){
        out=-errno;
    }
    else{
        out=ret2;
    }

    ret = margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&out);
    assert(ret==HG_SUCCESS);

    ret = margo_destroy(h);
    assert(ret==HG_SUCCESS); 
}
DEFINE_MARGO_RPC_HANDLER(open_s)

static void
read_s(hg_handle_t h){
    hg_return_t ret;
    read_in_t in;
    read_out_t out;

    printf("called read\n");

    ret = margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    int ret2;
    out.buf=malloc(in.size);

    ret2 = pread(in.fd,out.buf,in.size,in.off);

    if(ret2 == -1){
        out.error=-errno;
        out.n=0;
    }
    else{
        out.error=0;
        out.n=ret2;
    }

    ret = margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&out);
    assert(ret==HG_SUCCESS);

    free(out.buf);

    ret = margo_destroy(h);
    assert(ret==HG_SUCCESS); 
}
DEFINE_MARGO_RPC_HANDLER(read_s)

static void
read_rdma(hg_handle_t h){
    hg_return_t ret;
    read_rdma_in_t in;
    read_rdma_out_t out;

    printf("called read_rdma\n");

    ret = margo_get_input(h,&in);
    assert(ret==HG_SUCCESS);

    int ret2;
    char *buf=malloc(in.size);

    ret2 = pread(in.fd,buf,in.size,in.off);

    if(ret2 == -1){
        out.error=-errno;
        out.n=0;
    }
    else{
        out.error=0;
        out.n=ret2;

        hg_bulk_t bulk;

        margo_instance_id mid = margo_hg_handle_get_instance(h);

        const struct hg_info* info = margo_get_info(h);
        hg_addr_t client_addr = info->addr;
        hg_size_t buf_size = out.n;

        
        ret=margo_bulk_create(mid,1,(void**)&buf,&buf_size,HG_BULK_READ_ONLY,&bulk);
        assert(ret==HG_SUCCESS);

        ret=margo_bulk_transfer(mid,HG_BULK_PUSH,client_addr,in.buf,0,bulk,0,buf_size);
        assert(ret==HG_SUCCESS);

        ret=margo_bulk_free(bulk);
        assert(ret==HG_SUCCESS);
    }

    ret = margo_free_input(h,&in);
    assert(ret==HG_SUCCESS);

    ret=margo_respond(h,&out);
    assert(ret==HG_SUCCESS);

    free(buf);

    ret = margo_destroy(h);
    assert(ret==HG_SUCCESS); 
}
DEFINE_MARGO_RPC_HANDLER(read_rdma)