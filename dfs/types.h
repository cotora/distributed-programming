#include <mercury.h>
#include <mercury_macros.h>
#include <mercury_proc_string.h>

typedef struct timespec timespec_t;

MERCURY_GEN_STRUCT_PROC(timespec_t,
        ((int64_t)(tv_sec))((int64_t)(tv_nsec)))

MERCURY_GEN_PROC(stat_t,
        ((int32_t)(error))\
        ((uint64_t)(ino))\
        ((uint32_t)(mode))\
        ((uint64_t)(nlink))\
        ((uint32_t)(uid))\
        ((uint32_t)(gid))\
        ((int64_t)(size))\
        ((int64_t)(atime))\
        ((int64_t)(mtime))\
        ((int64_t)(ctime)))

MERCURY_GEN_PROC(dirent_t,
        ((uint64_t)(ino))\
        ((uint8_t)(type))\
        ((hg_string_t)(name)))

typedef struct dirents {
        int32_t error;
        int32_t n;
        dirent_t *d;
} dirents_t;

static inline hg_return_t
hg_proc_dirents_t(hg_proc_t proc, void *data)
{
        dirents_t *d = data;
        hg_return_t ret;
        int i;

        ret = hg_proc_int32_t(proc, &d->error);
        if (ret != HG_SUCCESS)
                return (ret);
        ret = hg_proc_int32_t(proc, &d->n);
        if (ret != HG_SUCCESS)
                return (ret);
        if (hg_proc_get_op(proc) == HG_DECODE)
                d->d = malloc(sizeof(dirent_t) * d->n);
        for (i = 0; i < d->n; ++i) {
                ret = hg_proc_dirent_t(proc, &d->d[i]);
                if (ret != HG_SUCCESS)
                        return (ret);
        }
        if (hg_proc_get_op(proc) == HG_FREE)
                free(d->d);
        return (ret);
}

MERCURY_GEN_PROC(open_in_t,
        ((hg_const_string_t)(path))\
        ((int32_t)(flags)))

MERCURY_GEN_PROC(read_in_t,
        ((int32_t)(fd))\
        ((int64_t)(size))\
        ((int64_t)(off)))

typedef struct read_out {
        int32_t error;
        int64_t n;
        char *buf;
} read_out_t;

static inline hg_return_t
hg_proc_read_out_t(hg_proc_t proc, void *data)
{
        read_out_t *d = data;
        hg_return_t ret;

        ret = hg_proc_int32_t(proc, &d->error);
        if (ret != HG_SUCCESS)
                return (ret);
        ret = hg_proc_int64_t(proc, &d->n);
        if (ret != HG_SUCCESS)
                return (ret);
        if (hg_proc_get_op(proc) == HG_DECODE)
                d->buf = malloc(d->n);
        ret = hg_proc_memcpy(proc, d->buf, d->n);
        if (ret != HG_SUCCESS)
                return (ret);
        if (hg_proc_get_op(proc) == HG_FREE)
                free(d->buf);
        return (ret);
}

MERCURY_GEN_PROC(read_rdma_in_t,
        ((int32_t)(fd))\
        ((int64_t)(size))\
        ((int64_t)(off))\
        ((hg_bulk_t)(buf)))

MERCURY_GEN_PROC(read_rdma_out_t,
        ((int32_t)(error))\
        ((int32_t)(n)))