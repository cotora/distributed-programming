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


static void AppendEntries(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(AppendEntries)

static void RequestVote(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(RequestVote)

static void InstallSnapshot(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(InstallSnapshot)

static void Submit(hg_handle_t h);
DECLARE_MARGO_RPC_HANDLER(Submit)

//ログを表す構造体
typedef struct logEntry{
    char *command;
    int term;
} logEntry;

static hg_return_t call_RequestVote(const char* addr,request_vote_out_t* out);
static hg_return_t call_AppendEntries(const char* addr,append_entries_out_t* out,int nLog,logEntry *entries,int prevLogIndex,int prevLogTerm);
static hg_return_t call_Submit(const char* addr,hg_string_t in,hg_bool_t *res);

//margoに関する情報を保持するための構造体
struct env {
    margo_instance_id mid;
    char *addr_str;
    hg_id_t AppendEntries_rpc,RequestVote_rpc,InstallSnapshot_rpc,Submit_rpc;
} env;

//サーバーの状態を表す型
typedef enum
{
    FOLLOWER,
    CANDIDATE,
    LEADER,
} S_STATE;

//状態を表す構造体
struct state{
    int currentTerm;
    int votedFor;
    int n;
    logEntry* log;
    int commitIndex;
    int lastApplied;
    int *nextIndex;
    int *matchIndex;
    S_STATE serverState;
    bool rpcFlag;
    double timeout;
    int id;
    int leaderId;
} state;

//永続的な状態を表す構造体(安定記憶装置との読み書きに使用する)
struct pstate{
    int currentTerm;
    int votedFor;
    int n;
    logEntry *log;
};

//設定を表す構造体
struct config{
    int n;
    char** server;
} config;

//ランダムなtimeoutの取得
double getRandomTimeout(){
    return 0.15+(rand()*0.15/RAND_MAX);
}

//状態をストレージから取得する関数
void getStateFromStorage(){
    FILE *fp;
    char *addr_path=strdup(env.addr_str);
    for(int i=0;i<strlen(addr_path);i++){
        if(addr_path[i]=='/')addr_path[i]='_';
    }
    fp=fopen(addr_path,"r");
    if(fp==NULL){
        fp=fopen(addr_path,"w");
        fprintf(fp,"%d %d\n",0,-1);
        fprintf(fp,"%d\n",0);
        fclose(fp);
        state.n=0;
        state.currentTerm=0;
        state.votedFor=-1;
    }
    else{
        fscanf(fp,"%d %d",&state.currentTerm,&state.votedFor);
        fscanf(fp,"%d",&state.n);
        state.log=malloc(sizeof(logEntry)*state.n);
        for(int i=0;i<state.n;i++){
            state.log[i].command=malloc(100);
        }
        for(int i=0;i<state.n;i++){
            fscanf(fp,"%d %s",&state.log[i].term,state.log[i].command);
        }
        fclose(fp);
    }

    free(addr_path);
}

//状態をストレージに保存する関数
void setStateToStorage(){
    char *addr_path=strdup(env.addr_str);
    for(int i=0;i<strlen(addr_path);i++){
        if(addr_path[i]=='/')addr_path[i]='_';
    }
    FILE *fp=fopen(addr_path,"w");
    if(fp!=NULL){
        fprintf(fp,"%d %d\n",state.currentTerm,state.votedFor);
        fprintf(fp,"%d\n",state.n);
        for(int i=0;i<state.n;i++){
            fprintf(fp,"%d %s\n",state.log[i].term,state.log[i].command);
        }
        fclose(fp);
    }

    free(addr_path);
}

//状態の初期化
void init_state(){
    //ストレージからの状態の取得
    getStateFromStorage();

    state.commitIndex=-1;
    state.lastApplied=-1;
    state.serverState=FOLLOWER;
    state.rpcFlag=false;
    state.timeout=getRandomTimeout();
    for(int i=0;i<config.n;i++){
        if(strcmp(config.server[i],env.addr_str)==0){
            state.id=i;
        }
    }
    state.nextIndex=malloc(sizeof(int)*config.n);
    state.matchIndex=malloc(sizeof(int)*config.n);
    state.leaderId=-1;
}

//設定の初期化
void init_config(){
    config.n=3;
    config.server=malloc(sizeof(char*)*config.n);
    char *tmp[]={"ofi+tcp://172.18.0.3:38255","ofi+tcp://172.18.0.2:38256","ofi+tcp://172.18.0.5:38257"};
    for(int i=0;i<3;i++){
        config.server[i]=strdup(tmp[i]);
    }
}

//ログの最後のエントリのindexとtermを返す
void lastLogIndexAndTerm(int *lastLogIndex,int *lastLogTerm){
    if(state.n>0){
        *lastLogIndex=state.n-1;
        *lastLogTerm=state.log[state.n-1].term;
    }
    else{
        *lastLogIndex=-1;
        *lastLogTerm=-1;
    }
}

//Followerに転向する処理
void becomeFollower(int term){
    state.currentTerm=term;
    state.votedFor=-1;
    state.serverState=FOLLOWER;
}

//Followerの動作
void startFollower(){
    printf("star follower\n");
    time_t prev_time=time(NULL);
    while(1){
        time_t cur_time=time(NULL);

        if(state.rpcFlag){
            //RPCが来ていたらタイマーをリセット
            prev_time=cur_time;
            printf("rpc detected\n");
            state.rpcFlag=false;
        }

        if(abs(cur_time-prev_time)>state.timeout){
            //タイムアウト時間が過ぎたらCandidateに転向
            state.serverState=CANDIDATE;
            return;
        }
    }
}

//選出の開始
void startElection(){
    printf("start election\n");

    //currentTermを1増やす
    state.currentTerm++;

    //ランダムなタイムアウト時間を設定
    double timeout=getRandomTimeout();
    double prevTime=time(NULL);

    state.votedFor=state.id;
    int votesReceived=1;

    //すべてのサーバーにRequestVote RPCを送信
    for(int i=0;i<config.n;i++){
        if(i==state.id)continue;

        request_vote_out_t out;
        call_RequestVote(config.server[i],&out);

        //レスポンスのtermがcurrentTermより大きければFollowerに転向する
        if(out.term>state.currentTerm){
            becomeFollower(out.term);
            return;
        }

        if(out.voteGranted && i!=state.id)votesReceived++;

        //タイムアウト時間を過ぎていたら新しい選挙を開始
        if(time(NULL)-prevTime>timeout){
            return;
        }
    }

    //Follower状態になっていたら選挙を中止
    if(state.serverState==FOLLOWER)return;

    //投票数が過半数以上なら当選し、Leader状態に転向する
    if(votesReceived>=(config.n/2)+1){
        state.serverState=LEADER;
        return;
    }

    while(time(NULL)-prevTime<timeout){
    }
}

//Candidateの動作
void startCandidate(){
    printf("start candidate\n");
    //Candidate状態である限り、選挙を繰り返す
    while(1){
        startElection();
        if(state.serverState!=CANDIDATE)break;
    }
}

//Leaderの動作
void startLeader(){
    printf("start leader\n");
    //nextIndexとmatchIndexを初期化
    state.nextIndex=realloc(state.nextIndex,sizeof(int)*config.n);
    state.matchIndex=realloc(state.matchIndex,sizeof(int)*config.n);
    for(int i=0;i<config.n;i++){
        state.nextIndex[i]=state.n;
        state.matchIndex[i]=-1;
    }

    while(1){
        margo_thread_sleep(env.mid,50);

        //各サーバにAppendEntries RPCを送信
        for(int i=0;i<config.n;i++){

            if(i==state.id)continue;

            append_entries_out_t out;

            //送るべきエントリがあった場合
            if(state.n-1>=state.nextIndex[i]){

                //prevLogIndexとprevLogTermの設定
                int prevLogIndex=state.nextIndex[i]-1;
                int prevLogTerm=-1;
                if(prevLogIndex>=0){
                    prevLogTerm=state.log[prevLogIndex].term;
                }

                //送るエントリ数とエントリの設定
                int nLog=state.n-state.nextIndex[i];
                logEntry *entries=malloc(sizeof(logEntry)*nLog);
                for(int j=0;j<nLog;j++){
                    entries[j]=state.log[state.nextIndex[i]+j];
                }

                //Append Entries RPCの送信
                call_AppendEntries(config.server[i],&out,nLog,entries,prevLogIndex,prevLogTerm);

                //成功した場合
                if(out.success){
                    //送信先のサーバーのnextIndexとmatchIndexを更新
                    state.nextIndex[i]+=nLog;
                    state.matchIndex[i]=state.nextIndex[i]-1;
                }
                //失敗した場合
                else{
                    //nextIndexをデクリメントする
                    state.nextIndex[i]--;
                }

                free(entries);
            }
            //送るべきエントリがない場合(空のAppend Entries)
            else{
                //Append Entries RPCの送信
                call_AppendEntries(config.server[i],&out,0,NULL,-1,-1);
            }

            //送信先のサーバのcurrentTermがリーダーのcurrentTermより大きい場合、Followerに転向する
            if(out.term>state.currentTerm){
                becomeFollower(out.term);
                return;
            }

            //N > commitIndex、過半数の matchIndex[i] ≧ N、log[N].term == currentTerm となる N が存在する場合: commitIndex = N に設定
            for(int j=state.n-1;j>=0;j--){
                if(j>state.commitIndex && state.log[j].term==state.currentTerm){
                    int matchCount=1;
                    for(int k=0;k<config.n;k++){
                        if(state.id==k)continue;
                        if(state.matchIndex[k]>=j)matchCount++;
                    }
                    if(matchCount>=(config.n/2)+1){
                        //commitIndexの更新
                        state.commitIndex=j;
                        break;
                    }
                }
            }

            while(state.commitIndex>state.lastApplied){
                state.lastApplied++;
            }

        }
    }
}

int
main(int argc, char *argv[])
{

        //必要な変数の宣言
        char addr_str[PATH_MAX];
        size_t addr_str_size = sizeof(addr_str);
        hg_addr_t my_address;

        //時間によるランダムなシード値の指定
        srand((unsigned int)time(NULL));

        if(argc<2){
            printf("アドレスを指定してください\n");
            return 0;
        }

        //margoインスタンスの初期化
        margo_instance_id mid = margo_init(argv[1], MARGO_SERVER_MODE, 1, 10);
        if (mid == MARGO_INSTANCE_NULL)
                fprintf(stderr, "margo_init failed, abort\n"), exit(1);

        //サーバーアドレスを取得して表示する
        margo_addr_self(mid, &my_address);
        margo_addr_to_string(mid, addr_str, &addr_str_size, my_address);
        margo_addr_free(mid, my_address);
        printf("Server running at address %s\n", addr_str);
        env.addr_str=addr_str;


        //状態の初期化
        init_state();

        //設定の初期化
        init_config();

        //RPCの登録
        env.mid = mid;
        env.AppendEntries_rpc = MARGO_REGISTER(mid, "AppendEntries", append_entries_in_t, append_entries_out_t,
                AppendEntries);
        env.RequestVote_rpc= MARGO_REGISTER(mid, "RequestVote", request_vote_in_t, request_vote_out_t,
                RequestVote);
        env.InstallSnapshot_rpc=MARGO_REGISTER(mid, "InstallSnapshot", install_snapshot_in_t, int32_t,
                InstallSnapshot);
        env.Submit_rpc=MARGO_REGISTER(mid, "Submit", hg_string_t, hg_bool_t,
                Submit);


        //Raftの動作を開始
        while(1){
            if(state.serverState==FOLLOWER){
                //Follower状態の開始
                startFollower();
            }
            else if(state.serverState==CANDIDATE){
                //Candidate状態の開始
                startCandidate();
            }
            else{
                //Leader状態の開始
                startLeader();
            }
        }
        
        margo_wait_for_finalize(mid);
        
        return (0);
}

//AppendEntries RPCを呼び出す関数
static hg_return_t
call_AppendEntries(const char *addr,append_entries_out_t *out2,int nLog,logEntry entries[],int prevLogIndex,int prevLogTerm){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;
    append_entries_in_t in;
    append_entries_out_t out;

    //引数を設定する
    in.term=state.currentTerm;
    in.leaderId=state.id;
    in.prevLogIndex=prevLogIndex;
    in.prevLogTerm=prevLogTerm;
    in.n=nLog;
    in.entries=malloc(sizeof(logEntry)*nLog);
    for(int i=0;i<nLog;i++){
        in.entries[i].term=entries[i].term;
        in.entries[i].command=strdup(entries[i].command);
    }
    in.leaderCommit=state.commitIndex;

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);

    ret=margo_create(env.mid,serv_addr,env.AppendEntries_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    //RPCを送信
    ret=margo_forward_timed(h,&in,20);
    if(ret!=HG_SUCCESS)
        goto err;

    for(int i=0;i<nLog;i++){
        free(in.entries[i].command);
    }

    free(in.entries);

    //RPCの応答を取得する
    ret=margo_get_output(h,&out);
    if(ret!=HG_SUCCESS)
        goto err;

    //応答結果を引数に代入する
    out2->term=out.term;
    out2->success=out.success;

    margo_free_output(h,&out);

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

//RequestVote RPCを呼び出す関数
static hg_return_t
call_RequestVote(const char *addr,request_vote_out_t *out2){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;
    request_vote_in_t in;
    request_vote_out_t out;

    //引数を設定する
    in.term=state.currentTerm;
    in.candidateId=state.id;
    lastLogIndexAndTerm(&in.lastLogIndex,&in.lastLogTerm);

    ret=margo_addr_lookup(env.mid,addr,&serv_addr);

    ret=margo_create(env.mid,serv_addr,env.RequestVote_rpc,&h);
    if(ret!=HG_SUCCESS)return (ret);

    //RPCを送信
    ret=margo_forward_timed(h,&in,20);
    if(ret!=HG_SUCCESS)
        goto err;

    //RPCの応答結果を取得する
    ret=margo_get_output(h,&out);
    if(ret!=HG_SUCCESS)
        goto err;

    //引数に応答結果を代入する
    out2->term=out.term;
    out2->voteGranted=out.voteGranted;

    margo_free_output(h,&out);

err:
    ret2=margo_destroy(h);
    if(ret==HG_SUCCESS)
        ret=ret2;
    return (ret);

}

//Submit RPCを呼び出す関数
static hg_return_t
call_Submit(const char *addr,hg_string_t in,hg_bool_t *res){
    hg_handle_t h;
    hg_return_t ret,ret2;
    hg_addr_t serv_addr;
    hg_bool_t out;

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

//AppendEntries　RPC
static void
AppendEntries(hg_handle_t h){
        hg_return_t ret;
        append_entries_in_t in;
        append_entries_out_t out;

        //printf("AppendEntries RPC\n");

        state.rpcFlag=true;

        //RPCの引数を取得する
        ret = margo_get_input(h, &in);
        assert(ret == HG_SUCCESS);

        //term<currentTermの場合はfalseと応答する
        if(in.term<state.currentTerm){
            out.term=state.currentTerm;
            out.success=false;
        }
        else{
            //送り主のtermがcurrentTermより大きい場合、Followerに転向する
            if(state.currentTerm<in.term){
                becomeFollower(in.term);
            }

            //サーバーの状態がCandidateのときは、termが等しくてもFollowerに転向する
            if(state.currentTerm==in.term && state.serverState!=FOLLOWER){
                becomeFollower(in.term);
            }

            out.term=state.currentTerm;

            state.leaderId=in.leaderId;

            //ログにprevLogIndex,prevLogTermと一致するエントリが含まれていない場合はfalseと応答する
            if(state.n<=in.prevLogIndex || (in.prevLogIndex!=-1 && state.log[in.prevLogIndex].term!=in.prevLogTerm)){
                out.success=false;
            }
            else{
                int baseIndex=in.prevLogIndex+1;
                for(int i=0;i<in.n;i++){
                    if(state.n>baseIndex+i && state.log[baseIndex+i].term!=in.term){
                        state.n=baseIndex+i;
                        state.log=realloc(state.log,state.n);
                    }
                    if(state.n<=baseIndex+i){
                        state.n++;
                        state.log=realloc(state.log,state.n);
                        state.log[state.n-1].command=strdup(in.entries[i].command);
                        state.log[state.n-1].term=in.entries[i].term;
                    }
                }

                out.success=true;
            }

            if(in.leaderCommit>state.commitIndex){
                state.commitIndex=(in.leaderCommit<(state.n-1) ? in.leaderCommit : (state.n-1));

                //ステートマシンに適用
                while(state.commitIndex>state.lastApplied){
                    state.lastApplied++;
                }
            }
        }

        //応答する前に状態をストレージに保存する
        setStateToStorage();

        //RPCの応答
        ret = margo_respond(h, &out);
        assert(ret == HG_SUCCESS);

        ret = margo_free_input(h, &in);
        assert(ret == HG_SUCCESS);

        ret = margo_destroy(h);
        assert(ret == HG_SUCCESS);
}
DEFINE_MARGO_RPC_HANDLER(AppendEntries)

//RequestVote RPC
static void
RequestVote(hg_handle_t h){
        hg_return_t ret;
        request_vote_in_t in;

        state.rpcFlag=true;

        printf("RequestVote RPC\n");

        //RPCの引数を取得する
        ret = margo_get_input(h, &in);
        assert(ret == HG_SUCCESS);

        request_vote_out_t out;

        //term<currentTermの場合はfalseと応答する
        if(in.term<state.currentTerm){
            out.term=state.currentTerm;
            out.voteGranted=false;
        }
        else{
            //送り主のtermがcurrentTermより大きい場合、Followerに転向する
            if(state.currentTerm<in.term){
                becomeFollower(in.term);
            }

            out.term=state.currentTerm;

            int lastLogIndex;
            int lastLogTerm;

            lastLogIndexAndTerm(&lastLogIndex,&lastLogTerm);

            //votedForが-1またはcandidateIdであり、候補者のログが少なくとも受信者のログと同じように最新のものである場合、投票を許可する
            if((state.votedFor==-1 || state.votedFor==in.candidateId) && (in.lastLogTerm>lastLogTerm || (in.lastLogTerm==lastLogTerm && in.lastLogIndex>=lastLogIndex))){
                    state.votedFor=in.candidateId;
                    out.voteGranted=true;
            }
            else{
                out.voteGranted=false;
            }
        }

        //応答する前に状態をストレージに保存する
        setStateToStorage();

        //RPCの応答
        ret=margo_respond(h,&out);
        assert(ret==HG_SUCCESS);

        ret = margo_free_input(h, &in);
        assert(ret == HG_SUCCESS);

        ret = margo_destroy(h);
        assert(ret == HG_SUCCESS);
}
DEFINE_MARGO_RPC_HANDLER(RequestVote)

static void
InstallSnapshot(hg_handle_t h){
        hg_return_t ret;
        install_snapshot_in_t in;

        ret = margo_get_input(h, &in);
        assert(ret == HG_SUCCESS);

        ret = margo_free_input(h, &in);
        assert(ret == HG_SUCCESS);

        ret = margo_destroy(h);
        assert(ret == HG_SUCCESS);
}
DEFINE_MARGO_RPC_HANDLER(InstallSnapshot)

//クライアントからのコマンドリクエストを受け取るRPC
static void
Submit(hg_handle_t h){
    hg_return_t ret;
    hg_string_t in;
    hg_bool_t out=false;

    printf("submit RPC\n");

    ret = margo_get_input(h, &in);
    assert(ret==HG_SUCCESS);

    //サーバーがLeaderの場合はコマンドをログに追加する
    if(state.serverState==LEADER || state.leaderId==state.id){
        out=true;

        printf("accepted command from client\n");

        int index=state.n;
        state.n++;
        state.log=realloc(state.log,state.n);
        state.log[state.n-1].term=state.currentTerm;
        state.log[state.n-1].command=strdup(in);

        //コマンドがコミットされたらクライアントに応答する
        while(1){
            if(state.lastApplied>=index){
                break;
            }
        }
    }
    //サーバーがLeader以外ならLeaderにリダイレクトする
    else if(state.serverState!=LEADER && state.leaderId!=-1){
        call_Submit(config.server[state.leaderId],in,&out);
    }

    ret = margo_respond(h, &out);

    ret = margo_free_input(h, &in);

    ret = margo_destroy(h);
    assert(ret == HG_SUCCESS);
}
DEFINE_MARGO_RPC_HANDLER(Submit)