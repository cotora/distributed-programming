#include <mercury.h>
#include <mercury_macros.h>
#include <mercury_proc_string.h>

MERCURY_GEN_PROC(logEntry_t,
    ((hg_string_t)(command))\
    ((int32_t)(term))
)

typedef struct append_entries_in{
    int32_t term;
    int32_t leaderId;
    int32_t prevLogIndex;
    int32_t prevLogTerm;
    int32_t n;
    logEntry_t *entries;
    int32_t leaderCommit;
} append_entries_in_t;

static inline hg_return_t
hg_proc_append_entries_in_t(hg_proc_t proc, void *data)
{
    append_entries_in_t *d=data;
    hg_return_t ret;
    int i;

    ret = hg_proc_int32_t(proc,&d->term);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->leaderId);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->prevLogIndex);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->prevLogTerm);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->n);
    if(ret != HG_SUCCESS)
        return (ret);

    if(hg_proc_get_op(proc)== HG_DECODE)
        d->entries = malloc(sizeof(logEntry_t) * d->n);
    for(i=0;i<d->n;++i) {
        ret=hg_proc_logEntry_t(proc,&d->entries[i]);
        if(ret != HG_SUCCESS)
            return (ret);
    }
    if(hg_proc_get_op(proc) == HG_FREE)
        free(d->entries);
    
    ret = hg_proc_int32_t(proc,&d->leaderCommit);
    if(ret != HG_SUCCESS)
        return (ret);

    return ret;
}

MERCURY_GEN_PROC(append_entries_out_t,
    ((int32_t)(term))\
    ((hg_bool_t)(success))
)

MERCURY_GEN_PROC(request_vote_in_t,
    ((int32_t)(term))\
    ((int32_t)(candidateId))\
    ((int32_t)(lastLogIndex))\
    ((int32_t)(lastLogTerm))
)

MERCURY_GEN_PROC(request_vote_out_t,
    ((int32_t)(term))\
    ((hg_bool_t)(voteGranted))
)

typedef struct install_snapshot_in{
    int32_t term;
    int32_t leaderId;
    int32_t lastIncludedIndex;
    int32_t lastIncludedTerm;
    int32_t offset;
    int32_t n;
    int32_t *data;
    hg_bool_t done;
} install_snapshot_in_t;

static inline hg_return_t
hg_proc_install_snapshot_in_t(hg_proc_t proc, void *data)
{
    install_snapshot_in_t *d=data;
    hg_return_t ret;
    int i;

    ret = hg_proc_int32_t(proc,&d->term);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->leaderId);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->lastIncludedIndex);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->lastIncludedTerm);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->offset);
    if(ret != HG_SUCCESS)
        return (ret);
    ret = hg_proc_int32_t(proc,&d->n);
    if(ret != HG_SUCCESS)
        return (ret);

    if(hg_proc_get_op(proc)== HG_DECODE)
        d->data = malloc(sizeof(int32_t) * d->n);
    for(i=0;i<d->n;++i) {
        ret=hg_proc_int32_t(proc,&d->data[i]);
        if(ret != HG_SUCCESS)
            return (ret);
    }
    if(hg_proc_get_op(proc) == HG_FREE)
        free(d->data);
    
    ret = hg_proc_hg_bool_t(proc,&d->done);
    if(ret != HG_SUCCESS)
        return (ret);

    return ret;
}







