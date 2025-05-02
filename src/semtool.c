
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include "channel_cnf.h"

#define BUFSIZE 1048

static int		G_SEM_ID = 0;
static key_t	G_KEY;

void print_warn()
{
	fprintf(stderr, "semtool [option]\n");
	fprintf(stderr, "-c : セマフォを割り当てる\n");
	fprintf(stderr, "-d : セマフォを削除する\n");
	fprintf(stderr, "-u : セマフォUNLOCKする\n");
	fprintf(stderr, "-h : 使い方を表示する\n");
}


/************************************************************************************
static int channel_filepath(char *file_path)
{
	char buf[1024];
	FILE *fp = NULL;
	ssize_t path_len;

	// 環境変数 $CHANNELCONFPATH/channel.cnf が存在する場合
	const char *env;
	env = getenv(CHANNEL_FILE_PATH);
	if(env!=NULL){
		snprintf(file_path, BUFSIZE, "%s/%s", env, CHANNEL_FILE);
		if((fp = fopen(file_path, "r")) == NULL ){
			//fprintf(stderr, "channel_filepath() %s errno:%d [%s]\n", file_path, errno, strerror(errno));
		}
	}

	// 環境変数 $CHANNELCONFPATH が未設定 または $CHANNELCONFPATH/channel.cnf
	// file read open に失敗した場合 ロードモジュールの存在するディレクトリ/channel.iniを
	// 読み込む
	if(fp==NULL){
		path_len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
		if(path_len==-1){
			perror("readlink()");
			return(CH_RETURN_NOTFOUND);
		}
		buf[path_len] = '\0';
		dirname(buf);
		snprintf(file_path, BUFSIZE, "%s/%s", buf, CHANNEL_FILE);
		if((fp = fopen(file_path, "r")) == NULL ){
			fprintf(stderr, "%s errno:%d [%s]\n", file_path, errno, strerror(errno));
			return(CH_RETURN_NOTFOUND);
		}else{
			fclose(fp);
		}
	}else{
		fclose(fp);
	}

	return(CH_RETURN_FOUND);
}

************************************************************************************/

static void ipc_init()
{
	// 環境変数 $CHANNELCONFKEYが設定されている場合
	const char *env;
	env = getenv(CHANNEL_IPC_KEY);
	if(env==NULL){
		G_KEY = IPC_DEFAULT_KEY;
	}else{
		G_KEY = strtoul(env, NULL, 0);
	}

	return;
}

static int semaphore_lock(uint8_t lock)
{
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = (lock == SEMAPHORE_LOCK)?-1:1;
	sb.sem_flg = 0;

	G_SEM_ID = semget(G_KEY, 0, 0);
	if(G_SEM_ID != -1){
		if(semop(G_SEM_ID, &sb, 1) == -1){
			fprintf(stderr, "セマフォ%s失敗 errno : %d %s\n", (lock == SEMAPHORE_LOCK)?"LOCK":"UNLOCK", errno, strerror(errno));
			return(0);  
		}
	}else{
		fprintf(stderr, "セマフォ%s ID取得失敗 errno : %d %s\n", (lock == SEMAPHORE_LOCK)?"LOCK":"UNLOCK", errno, strerror(errno));
		return(0);  
	}

	return(1);
}

static int semaphore_delete()
{
	int rtn = -1;

	G_SEM_ID = semget(G_KEY, 0, 0);
	if(G_SEM_ID != -1){
		rtn = semctl(G_SEM_ID, 0, IPC_RMID);
		if(rtn < 0){
			fprintf(stderr, "セマフォ削除失敗 errno : %d %s\n", errno, strerror(errno));
		}
	}else{
		fprintf(stderr, "セマフォID取得失敗 errno : %d %s\n", errno, strerror(errno));
	}
	return((rtn < 0) ? 0 : 1);
}

static int semaphore_create()
{
	int rtn = 0;

	G_SEM_ID = semget(G_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
    if(G_SEM_ID == -1){
		if(errno == EEXIST){
			fprintf(stderr, "セマフォ割当 既に割当済み errno : %d %s\n", errno, strerror(errno));
			rtn = semaphore_delete();
			if(rtn){
				G_SEM_ID = semget(G_KEY, 1, IPC_CREAT | 0666);
    			if(G_SEM_ID == -1){
					fprintf(stderr, "セマフォ割当失敗 errno : %d %s\n", errno, strerror(errno));
				}else{
					rtn = 1;
				}
			}
		}
	}else{
		rtn = 1;
	}

	return(rtn);
}


int main(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	int ret;

	if(argc != 2){
		print_warn();
		return(0);
	}


	ipc_init();

	while ((opt = getopt(argc, argv, "cduh")) != -1) {
		switch(opt){
		case 'c':
			ret = semaphore_create();
			fprintf(stderr, "セマフォ割当%s\n", (ret==1) ? "成功" : "失敗" );
			break;
		case 'd':
			ret = semaphore_delete();
			fprintf(stderr, "セマフォ削除%s\n", (ret==1) ? "成功" : "失敗" );
			break;
		case 'u':
			ret = semaphore_lock(SEMAPHORE_UNLOCK);
			fprintf(stderr, "セマフォUNLOCK %s\n", (ret==1) ? "成功" : "失敗" );
			break;
		case 'h':
			print_warn();
			break;
		default:
			print_warn();
			break;
		}
	}
	return(1);
}

