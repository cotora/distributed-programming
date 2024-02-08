#define FUSE_USE_VERSION 26
#include <fuse.h>
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
#include "types.h"

struct env {
        margo_instance_id mid;
        hg_addr_t addr;
        hg_id_t getattr_rpc, readdir_rpc, open_rpc, read_rpc, read_rdma_rpc;
} env;

void
copy_stat(stat_t *s1,struct stat *s2){
    s2->st_ino=s1->ino;
    s2->st_mode=s1->mode;
    s2->st_nlink=s1->nlink;
    s2->st_uid=s1->uid;
    s2->st_gid=s1->gid;
    s2->st_size=s1->size;
    s2->st_atime=s1->atime;
    s2->st_mtime=s1->mtime;
    s2->st_ctime=s1->ctime;
}

int
dfs_init(const char *server)
{
        hg_return_t ret;

        /* the 3rd argument should be 1 */
        margo_instance_id mid = margo_init("tcp", MARGO_CLIENT_MODE, 1, 3);
        assert(mid);

        env.mid = mid;
        ret = margo_addr_lookup(mid, server, &env.addr);
        if (ret != HG_SUCCESS)
                fprintf(stderr, "%s: %s, abort\n", server,
                    HG_Error_to_string(ret)), exit(1);
        env.getattr_rpc = MARGO_REGISTER(mid, "getattr", hg_string_t, stat_t,
                NULL);
        env.readdir_rpc = MARGO_REGISTER(mid, "readdir", hg_string_t, dirents_t,
                NULL);
        env.open_rpc = MARGO_REGISTER(mid, "open", open_in_t, int32_t, NULL);
        env.read_rpc = MARGO_REGISTER(mid, "read", read_in_t, read_out_t, NULL);
        env.read_rdma_rpc = MARGO_REGISTER(mid, "read_rdma",
                read_rdma_in_t, read_rdma_out_t, NULL);

        return (0);
}

static void *
dfuse_init(struct fuse_conn_info *conn)
{
        struct fuse_context *ctx = fuse_get_context();
        const char *server = ctx->private_data;

        dfs_init(server);
        return (NULL);
}

static int
dfuse_getattr(const char *path, struct stat *st)
{
    hg_handle_t h;
    hg_return_t ret,ret2;
    int error;

    ret=margo_create(env.mid,env.addr,env.getattr_rpc,&h);
    if(ret!=HG_SUCCESS)return -1;

    ret=margo_forward(h,&path);
    if(ret!=HG_SUCCESS)
        goto err;

    stat_t resp;
    ret=margo_get_output(h,&resp);
    if(ret!=HG_SUCCESS)
        goto err;

    error=resp.error;
    copy_stat(&resp,st);
    margo_free_output(h,&resp);

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    if(ret!=HG_SUCCESS)return -1;

    if(error>=0){
        return 0;
    }
    else{
        return error;
    }

}

static int
dfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi)
{
    hg_handle_t h;
    hg_return_t ret,ret2;
    struct stat sb;
    int error;

    ret=margo_create(env.mid,env.addr,env.readdir_rpc,&h);
    if(ret!=HG_SUCCESS)return -1;

    ret=margo_forward(h,&path);
    if(ret!=HG_SUCCESS)
        goto err;

    dirents_t resp;
    ret=margo_get_output(h,&resp);
    if(ret!=HG_SUCCESS)
        goto err;

    error=resp.error;
    if(error>=0){
        for(int i=0;i<resp.n;i++){
            memset(&sb,0,sizeof(sb));
            sb.st_ino=resp.d[i].ino;
            sb.st_mode=resp.d[i].type<<12;
            if(filler(buf,resp.d[i].name,&sb,0))
                break;
        }
    }
    margo_free_output(h,&resp);


err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    if(ret!=HG_SUCCESS)return -1;

    if(error>=0){
        return 0;
    }
    else{
        return error;
    }
}

static int
dfuse_open(const char *path, struct fuse_file_info *fi)
{
    hg_handle_t h;
    hg_return_t ret,ret2;
    int error;
    int fd;

    ret=margo_create(env.mid,env.addr,env.open_rpc,&h);
    if(ret!=HG_SUCCESS)return -1;

    open_in_t in;
    in.path=path;
    in.flags=fi->flags;
    ret=margo_forward(h,&in);
    if(ret!=HG_SUCCESS)
        goto err;


    int32_t resp;
    ret=margo_get_output(h,&resp);
    if(ret!=HG_SUCCESS)
        goto err;

    error=resp;
    fd=resp;
    margo_free_output(h,&resp);


err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    if(ret!=HG_SUCCESS)return -1;

    if(error>=0){
        fi->fh=fd;
        return 0;
    }
    else{
        return error;
    }
}

static int
dfuse_read(const char *path, char *buf, size_t size, off_t off,
        struct fuse_file_info *fi)
{
    hg_handle_t h;
    hg_return_t ret,ret2;
    int error;
    int fd;
    int n;

    if(fi==NULL){
        struct fuse_file_info fi2;
        fi2.flags=O_RDONLY;
        int res=dfuse_open(path,&fi2);
        if(res!=0)return res;
        fd=fi2.fh;
    }
    else{
        fd=fi->fh;
    }

    //rdmaではない場合
    ret=margo_create(env.mid,env.addr,env.read_rpc,&h);
    if(ret!=HG_SUCCESS)return -1;

    read_in_t in;
    in.fd=fd;
    in.size=size;
    in.off=off;
    ret=margo_forward(h,&in);
    if(ret!=HG_SUCCESS)
        goto err;

    read_out_t resp;
    ret=margo_get_output(h,&resp);
    if(ret!=HG_SUCCESS)
        goto err;

    error=resp.error;
    n=resp.n;
    if(error>=0){
        for(int i=0;i<resp.n;i++){
            buf[i]=resp.buf[i];
        }
    }
    margo_free_output(h,&resp);

    /*
    //rdmaの場合
    ret=margo_create(env.mid,env.addr,env.read_rdma_rpc,&h);
    if(ret!=HG_SUCCESS)return -1;

    read_rdma_in_t in;
    in.fd=fd;
    in.size=size;
    in.off=off;

    ret=margo_bulk_create(env.mid,1,(void**)&buf,(hg_size_t*)&size,HG_BULK_WRITE_ONLY,&in.buf);
    if(ret!=HG_SUCCESS)
        goto err;

    ret=margo_forward(h,&in);
    if(ret!=HG_SUCCESS)
        goto err;

    read_rdma_out_t resp;
    ret=margo_get_output(h,&resp);
    if(ret!=HG_SUCCESS)
        goto err;

    error=resp.error;
    n=resp.n;

    margo_bulk_free(in.buf);
    margo_free_output(h,&resp);
    */
    
err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    if(ret!=HG_SUCCESS)return -1;

    if(error>=0){
        return n;
    }
    else{
        return error;
    }
}


static struct options {
        const char *server;
        int usage;
} options;

#define OPTION(t, p) { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
        OPTION("--server=%s", server),
        OPTION("-h", usage),
        OPTION("--help", usage),
        FUSE_OPT_END
};

static void usage(const char *progname)
{
        printf("usage: %s [options] <mountpoint>\n\n", progname);
        printf("file-system specific options:\n"
               "    --server=<s>        server name\n"
               "\n");
}

static const struct fuse_operations dfs_op = {
        .init           = dfuse_init,
        .getattr        = dfuse_getattr,
        .readdir        = dfuse_readdir,
        .open           = dfuse_open,
        .read           = dfuse_read,
};

int
main(int argc, char *argv[])
{
        int ret;
        struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

        options.server = NULL;
        if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
                return (1);

        if (options.usage || options.server == NULL) {
                usage(argv[0]);
                fuse_opt_add_arg(&args, "-ho");
        }

        ret = fuse_main(args.argc, args.argv, &dfs_op, (void *)options.server);
        fuse_opt_free_args(&args);
        return (ret);
}