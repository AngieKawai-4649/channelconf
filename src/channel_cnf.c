#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>

#include "channel_cnf.h"

#define BUFSIZE 1048

static int		G_SEM_ID = 0;
static int		G_SHM_ID = 0;
static key_t	G_KEY;
static uint32_t	*G_SHM_ADR = NULL;
static char		G_FILE_PATH[BUFSIZE];

char *ltrim(char *buf, uint8_t cnt, ... )
{
	if(buf==NULL){
		return(NULL);
	}
	if(!strlen(buf) || cnt==0){
		return(buf);
	}

	va_list arg_ptr;
	char *tm = (char *)malloc(cnt);
	if(tm==NULL){
		return(NULL);
	}

	va_start(arg_ptr, cnt);

	for(uint8_t i = 0; i < cnt; i++ ){
		*(tm+i) = (char) va_arg(arg_ptr, uint32_t);
	}
	va_end(arg_ptr);

	bool found;
	char *cp = buf;
	for(uint32_t i = 0; *cp!='\0'; cp++,i++){
		found = false;
		for(uint32_t k=0; k < cnt; k++){
			if(*cp ==*(tm+k)){
				found = true;
				break;
			} 
		}
		if(!found){
			break;
		}
	}

	free(tm);
	return(cp);
}


char *rtrim(char *buf, uint8_t cnt, ... )
{
	if(buf==NULL){
		return(NULL);
	}
	if(!strlen(buf) || cnt==0){
		return(buf);
	}

	va_list arg_ptr;
	char *tm = (char *)malloc(cnt);
	if(tm==NULL){
		return(NULL);
	}

	va_start(arg_ptr, cnt);

	for(uint8_t i = 0; i < cnt; i++ ){
		*(tm+i) = (char) va_arg(arg_ptr, uint32_t);
	}
	va_end(arg_ptr);

	uint32_t buflen = strlen(buf);
	char *cp = buf + buflen -1;
	bool found;
	for(uint32_t i = 0; i < buflen; cp--, i++){
		found = false;
		for(uint32_t k=0; k < cnt; k++){
			if(*cp ==*(tm+k)){
				*(cp) = '\0';
				found = true;
				break;
			}
		}
		if(!found){
			break;
		}
	}

	free(tm);
	return(buf);
}

char *comment_cut(char *buf, int cm)
{
	char *cp;

	cp = strchr(buf, cm);
	if(cp!=NULL){
		*cp = '\0';
	}
	return(buf);
}

static int semaphore_lock(uint8_t lock)
{
	struct sembuf sb;
	sb.sem_num = 0;
	sb.sem_op = (lock == SEMAPHORE_LOCK)?-1:1;
	sb.sem_flg = 0;

	if(semop(G_SEM_ID, &sb, 1) == -1){
		fprintf(stderr, "semaphore_lock() セマフォ%s失敗 errno : %d %s\n", (lock == SEMAPHORE_LOCK)?"LOCK":"UNLOCK", errno, strerror(errno));
		return(0);  
	}
	return(1);
}

void print_channel_cnt(FILE *fp)
{
	semaphore_lock(SEMAPHORE_LOCK);
	CHANNEL_CNT *ch_cnt = (CHANNEL_CNT *)(G_SHM_ADR + 1);
	fprintf(fp, "地デジ cnt : %u  offset %u\n", ch_cnt->cnt, ch_cnt->offset);
	fprintf(fp, "CATV   cnt : %u  offset %u\n", (ch_cnt+1)->cnt, (ch_cnt+1)->offset);
	fprintf(fp, "BS     cnt : %u  offset %u\n", (ch_cnt+2)->cnt, (ch_cnt+2)->offset);
	fprintf(fp, "CATV   cnt : %u  offset %u\n", (ch_cnt+3)->cnt, (ch_cnt+3)->offset);
	semaphore_lock(SEMAPHORE_UNLOCK);
}

void print_channel_info(FILE *fp, CHANNEL_INFO *ch_info)
{
	fprintf(fp, "-------------------------------------\n");
	fprintf(fp, "tuning_space : %u\n", ch_info->tuning_space);
	fprintf(fp, "channel_key  : %s\n", ch_info->channel_key);
	fprintf(fp, "channel      : %s\n", ch_info->channel);
	fprintf(fp, "l_tsid       : %u\n", ch_info->l_tsid);
	fprintf(fp, "pt_ch        : %u\n", ch_info->pt_ch);
	fprintf(fp, "freq         : %u\n", ch_info->freq);
	fprintf(fp, "sid          : %u\n", ch_info->sid);
	fprintf(fp, "tsid         : 0x%x\n", ch_info->tsid);
	fprintf(fp, "view         : %u\n", ch_info->view);
	fprintf(fp, "mirakurun    : %u\n", ch_info->mirakurun);
	fprintf(fp, "ch_name      : %s\n", ch_info->ch_name);
}

static int set_channel_info( char *buf, CHANNEL_INFO *ch_info)
{
	char *cp = buf;
	char *found;
	char *cnf[8];
	uint8_t i;

	for(i = 0; i < 8; i++){
		cnf[i] = cp;
		found = strchr(cp, ',');
		if(found == NULL){
			break;
		}else{
			*found = '\0';	
		}
		cp = ++found;
	}

	if( i != 7 ){
		return(0);
	}

	memset(ch_info, '\0', sizeof(CHANNEL_INFO));

	// CH
	cnf[0] = ltrim(cnf[0], 4, ' ', '\t', '\r', '\n');
	cnf[0] = rtrim(cnf[0], 4, ' ', '\t', '\r', '\n');
	strcpy(ch_info->channel_key, cnf[0]);

	// tuning space
	// channel
	// l_tsid
	if(!strncmp(ch_info->channel_key, SP_UHF, strlen(SP_UHF))){
		ch_info->tuning_space = SPI_UHF;
		ch_info->l_tsid = 0;
		cp = strchr(cnf[0], '_');
		if(cp!=NULL){
			strcpy(ch_info->channel, ++cp);
		}
	}else if(!strncmp(ch_info->channel_key, SP_CATV, strlen(SP_CATV))){
        ch_info->tuning_space = SPI_CATV;
		ch_info->l_tsid = 0;
		cp = strchr(cnf[0], '_');
		if(cp!=NULL){
			strcpy(ch_info->channel, ++cp);
		}
	}else if(!strncmp(ch_info->channel_key, SP_BS, strlen(SP_BS))){
        ch_info->tuning_space = SPI_BS;
		ch_info->l_tsid = 0;
		cp = strchr(cnf[0], '_');
		if(cp!=NULL){
			char *cp2 = strchr(++cp, '_');
			if(cp2!=NULL){
				*(cp2) = '\0';
				strcpy(ch_info->channel, cp);
				ch_info->l_tsid = strtoul(++cp2, NULL, 0);
			}
		}
	}else if(!strncmp(ch_info->channel_key, SP_CS, strlen(SP_CS))){
        ch_info->tuning_space = SPI_CS;
		ch_info->l_tsid = 0;
		cp = strchr(cnf[0], '_');
		if(cp!=NULL){
			strcpy(ch_info->channel, ++cp);
		}
	}

	// PT-CH
	cnf[1] = ltrim(cnf[1], 4, ' ', '\t', '\r', '\n');
	cnf[1] = rtrim(cnf[1], 4, ' ', '\t', '\r', '\n');
	ch_info->pt_ch = strtoul(cnf[1], NULL, 0);

	// FREQ
	cnf[2] = ltrim(cnf[2], 4, ' ', '\t', '\r', '\n');
	cnf[2] = rtrim(cnf[2], 4, ' ', '\t', '\r', '\n');
	ch_info->freq = (uint32_t)strtod(cnf[2], NULL) * 1000;

	// SID
	cnf[3] = ltrim(cnf[3], 4, ' ', '\t', '\r', '\n');
	cnf[3] = rtrim(cnf[3], 4, ' ', '\t', '\r', '\n');
	ch_info->sid = strtoul(cnf[3], NULL, 0);

	// TSID
	cnf[4] = ltrim(cnf[4], 4, ' ', '\t', '\r', '\n');
	cnf[4] = rtrim(cnf[4], 4, ' ', '\t', '\r', '\n');
	ch_info->tsid = strtoul(cnf[4], NULL, 0);

	// VIEW
	cnf[5] = ltrim(cnf[5], 4, ' ', '\t', '\r', '\n');
	cnf[5] = rtrim(cnf[5], 4, ' ', '\t', '\r', '\n');
	ch_info->view = strtoul(cnf[5], NULL, 0);

	// MIRAKURUN
	cnf[6] = ltrim(cnf[6], 4, ' ', '\t', '\r', '\n');
	cnf[6] = rtrim(cnf[6], 4, ' ', '\t', '\r', '\n');
	ch_info->mirakurun = strtoul(cnf[6], NULL, 0);

	// NAME
	cnf[7] = ltrim(cnf[7], 5, ' ', '"', '\t', '\r', '\n');
	cnf[7] = rtrim(cnf[7], 5, ' ', '"', '\t', '\r', '\n');
	strcpy(ch_info->ch_name, cnf[7]);

	return(1);
	
}

static int channel_filepath()
{
	char buf[1024];
	FILE *fp = NULL;
	ssize_t path_len;

	// 環境変数 $CHANNELCONFPATH/channel.cnf が存在する場合
	const char *env;
	env = getenv(CHANNEL_FILE_PATH);
	if(env!=NULL){
		snprintf(G_FILE_PATH, sizeof(G_FILE_PATH), "%s/%s", env, CHANNEL_FILE);
		if((fp = fopen(G_FILE_PATH, "r")) == NULL ){
			//fprintf(stderr, "channel_filepath() %s errno:%d [%s]\n", G_FILE_PATH, errno, strerror(errno));
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
		snprintf(G_FILE_PATH, sizeof(G_FILE_PATH), "%s/%s", buf, CHANNEL_FILE);
		if((fp = fopen(G_FILE_PATH, "r")) == NULL ){
			fprintf(stderr, "channel_filepath() %s errno:%d [%s]\n", G_FILE_PATH, errno, strerror(errno));
			return(CH_RETURN_NOTFOUND);
		}else{
			fclose(fp);
		}
	}else{
		fclose(fp);
	}

	return(CH_RETURN_FOUND);
}

int read_ch_cnt(CHANNEL_CNT *ch_cnt)
{
	FILE *fp;
	char read_buf[BUFSIZE];
	struct  {
		const char *sp_ch;
		uint8_t spi_ch;
	} sp_ch[] = {{SP_UHF,SPI_UHF}, {SP_CATV, SPI_CATV}, {SP_BS,SPI_BS}, {SP_CS, SPI_CS}};

	if((fp = fopen(G_FILE_PATH, "r"))==NULL){
		return(0);
	}

	memset(ch_cnt, '\0', sizeof(CHANNEL_CNT)*4);
	char *cp;
	while(fgets(read_buf, BUFSIZE, fp ) != NULL){
		cp = ltrim(read_buf, 4, ' ', '\t', '\r', '\n');
		cp = rtrim(cp, 4, ' ', '\t', '\r', '\n');
		cp = comment_cut(cp, ';');
		if(*cp!='\0'){

			for(uint8_t i = 0; i < 4; i++){
				if(!strncmp(cp, sp_ch[i].sp_ch, strlen(sp_ch[i].sp_ch))){
					(ch_cnt+i)->cnt++;
					break;
				}
			}
		}
	}
	uint32_t oset = 0;
	for(uint8_t i = 0; i < 4; i++){
		(ch_cnt+i)->offset = oset;
		oset += (ch_cnt+i)->cnt;
	}

	int rtn = feof(fp) == 0 ? 0 : 1;
	fclose(fp);
	return(rtn);
}

int get_ch_cnt(CHANNEL_CNT *ch_cnt)
{
	int rtn = semaphore_lock(SEMAPHORE_LOCK);
	if(rtn){
		memcpy(ch_cnt, G_SHM_ADR + 1, sizeof(CHANNEL_CNT) * 4);
		semaphore_lock(SEMAPHORE_UNLOCK);
	}
	return(rtn);
}

int set_ch_cnt(CHANNEL_CNT *ch_cnt)
{
	int rtn = semaphore_lock(SEMAPHORE_LOCK);
	if(rtn){
		memcpy(G_SHM_ADR + 1, ch_cnt, sizeof(CHANNEL_CNT) * 4);
		semaphore_lock(SEMAPHORE_UNLOCK);
	}
	return(rtn);
}

int read_ch_info()
{
	FILE *fp;
	int ret;
	int counter[4] = {0,0,0,0};
	char read_buf[BUFSIZE];
	CHANNEL_CNT *ch_cnt = (CHANNEL_CNT *)(G_SHM_ADR + 1);
	CHANNEL_INFO *ch_info = (CHANNEL_INFO *)(ch_cnt + 4);

	if((fp = fopen(G_FILE_PATH, "r"))==NULL){
		return(0);
	}
	char *cp;
	while(fgets(read_buf, BUFSIZE, fp ) != NULL){
		cp = ltrim(read_buf, 4, ' ', '\t', '\r', '\n');
		cp = rtrim(cp, 4, ' ', '\t', '\r', '\n');
		cp = comment_cut(cp, ';');
		if(*cp!='\0'){

			CHANNEL_INFO w;
			ret = set_channel_info(cp, &w);
			if(ret==0){
				return(0);
			}

			uint32_t oset;
			switch(w.tuning_space){
			case SPI_UHF:
				oset = ch_cnt->offset + counter[0];
				counter[0]++;
				break;
			case SPI_CATV:
				oset = (ch_cnt+1)->offset + counter[1];
				counter[1]++;
				break;
			case SPI_BS:
				oset = (ch_cnt+2)->offset + counter[2];
				counter[2]++;
				break;
			case SPI_CS:
				oset = (ch_cnt+3)->offset + counter[3];
				counter[3]++;
				break;
			}

			memcpy(ch_info+oset, &w, sizeof(CHANNEL_INFO));
		}
	}

	int rtn = feof(fp) == 0 ? 0 : 1;
	fclose(fp);
	return(rtn);
}

int ipc_init()
{
	if(channel_filepath()==CH_RETURN_NOTFOUND){
		fprintf(stderr, "ipc_init() channel.cnf ファイルパス取得失敗\n");
		return(0);
	}

	// 環境変数 $CHANNELCONFKEYが設定されている場合
	const char *env;
	env = getenv(CHANNEL_IPC_KEY);
	if(env==NULL){
		G_KEY = IPC_DEFAULT_KEY;
	}else{
		G_KEY = strtoul(env, NULL, 0);
	}

/************************************************************************
	G_KEY = ftok(G_FILE_PATH, SHM_KEY_ID);
	if(G_KEY == -1){
		fprintf(stderr, "ftok() %s key取得失敗 errno:%d %s\n", G_FILE_PATH, errno, strerror(errno));
		return(0);
	}
************************************************************************/
	return(1);
}

int semaphore_init()
{
	union semun arg;
	int rtn = 1;

	G_SEM_ID = semget(G_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
    if(G_SEM_ID == -1){
		if(errno == EEXIST){
			G_SEM_ID = semget(G_KEY, 0, 0);
    		if(G_SEM_ID == -1){
				fprintf(stderr, "semaphore_init() セマフォ 既に割当済み セマフォID取得失敗 errno : %d %s\n", errno, strerror(errno));
				return -2;  
			}
			return(0);
		}else{
			fprintf(stderr, "semaphore_init() セマフォ 新規割当 semget() セマフォ割当失敗 errno : %d %s\n", errno, strerror(errno));
			return -2;  
		}
    }
	arg.val = 1;
	rtn = semctl(G_SEM_ID, 0, SETVAL, arg);
	if(rtn < 0){
		fprintf(stderr, "semaphore_init() セマフォ初期化失敗 errno : %d %s\n", errno, strerror(errno));
		return -3;  
	}

	return(rtn);
}

int shared_memory_init(size_t *cs)
{
	int rtn;
	CHANNEL_CNT ch_cnt[4];

	rtn = semaphore_lock(SEMAPHORE_LOCK);
	if(!rtn){
		return(-2);
	}

	rtn = 0;

	G_SHM_ID = shmget(G_KEY, 0, 0);
    if(G_SHM_ID == -1){
		if(errno == ENOENT){
			memset(ch_cnt, '\0', sizeof(ch_cnt));
			if(read_ch_cnt(ch_cnt) == 0){
				semaphore_lock(SEMAPHORE_UNLOCK);
				return(-1);
			}

			G_SHM_ID = shmget(G_KEY,	sizeof(uint32_t) +
										sizeof(ch_cnt) +
										sizeof(CHANNEL_INFO) * (ch_cnt[0].cnt + ch_cnt[1].cnt + ch_cnt[2].cnt + ch_cnt[3].cnt), IPC_CREAT | 0666);
    		if(G_SHM_ID == -1){
				fprintf(stderr, "shared_memory_init() 共有メモリ未割当 共有メモリ割当失敗 errno : %d %s\n", errno, strerror(errno));
				semaphore_lock(SEMAPHORE_UNLOCK);
				return(-2);
			}
			rtn = 1;
		}else{
			fprintf(stderr, "shared_memory_init()  共有メモリID取得失敗 errno : %d %s\n", errno, strerror(errno));
			semaphore_lock(SEMAPHORE_UNLOCK);
			return(-2);
		}
	}

	G_SHM_ADR = (uint32_t *)shmat(G_SHM_ID, NULL, 0);
	if(G_SHM_ADR==(void *)-1){
		fprintf(stderr, "shared_memory_init() 共有メモリアドレス取得失敗 errno : %d %s\n", errno, strerror(errno));
		semaphore_lock(SEMAPHORE_UNLOCK);
		return(-2);
	}

	// 共有メモリ新規割当時、channel.cnf から共有メモリに展開
	struct shmid_ds buf;
	if(rtn == 1){
		*G_SHM_ADR = CH_LOG_OFF;

		memcpy(G_SHM_ADR+1, ch_cnt, sizeof(ch_cnt));
		if(read_ch_info() == 0){
			fprintf(stderr, "shared_memory_init()  共有メモリ新規割当　ファイルから展開 失敗\n");
			semaphore_lock(SEMAPHORE_UNLOCK);
			return(-1);
		}

		if(shmctl(G_SHM_ID, SHM_LOCK, &buf) == -1){
			fprintf(stderr, "shared_memory_init()  共有メモリLOCK失敗 errno : %d %s\n", errno, strerror(errno));
			semaphore_lock(SEMAPHORE_UNLOCK);
			return(-2);
		}

	}

	if(shmctl(G_SHM_ID, SHM_STAT, &buf) == -1){
		fprintf(stderr, "shared_memory_init() 共有メモリ情報取得失敗 errno : %d %s\n", errno, strerror(errno));
		semaphore_lock(SEMAPHORE_UNLOCK);
		return(-2);
	}

	*cs = buf.shm_segsz;
	semaphore_lock(SEMAPHORE_UNLOCK);
	return(rtn);
}


int shm_delete()
{
	int ret = semaphore_lock(SEMAPHORE_LOCK);
	if(!ret){
		return(ret);
	}

	ret = (shmctl(G_SHM_ID, IPC_RMID, NULL) == -1) ? 0 : 1;
	if(!ret){
		fprintf(stderr, "共有メモリ削除失敗 errno:%d %s\n", errno, strerror(errno));
	}
	semaphore_lock(SEMAPHORE_UNLOCK);

	return(ret);
}



int search_tsid(uint32_t tsid, uint32_t cnt, CHANNEL_INFO *output)
{
	int rtn = CH_RETURN_NOTFOUND;
	uint32_t max;
	CHANNEL_CNT *ch_cnt;
	CHANNEL_INFO *ch_info;

	if(semaphore_lock(SEMAPHORE_LOCK)==0){
		return(rtn);
    }

	ch_cnt = (CHANNEL_CNT *)(G_SHM_ADR + 1);
	ch_info = (CHANNEL_INFO *)(ch_cnt + 4);
	max = ch_cnt->cnt + (ch_cnt+1)->cnt + (ch_cnt+2)->cnt + (ch_cnt+3)->cnt;

	for(uint32_t i=0; i < max; i++, ch_info++){
		if(ch_info->tsid == tsid){
			if(i+cnt < max){ 
				if((ch_info+cnt)->tsid == tsid){
					memcpy(output, (ch_info+cnt), sizeof(CHANNEL_INFO));
					rtn = CH_RETURN_FOUND;
				}
			}
			break;
		}
	}
	semaphore_lock(SEMAPHORE_UNLOCK);
	return(rtn);
}

int search_channel_key(char *ch_key, uint32_t cnt, CHANNEL_INFO *output)
{
	uint8_t ch_no = 0;
	int rtn = CH_RETURN_NOTFOUND;
	CHANNEL_CNT *ch_cnt;
	CHANNEL_INFO *ch_info;

	if(strncmp(ch_key, SP_UHF, strlen(SP_UHF))==0){
		ch_no = SPI_UHF;
	}else if(strncmp(ch_key, SP_CATV, strlen(SP_CATV))==0){
		ch_no = SPI_CATV;
	}else if(strncmp(ch_key, SP_BS, strlen(SP_BS))==0){
		ch_no = SPI_BS;
	}else if(strncmp(ch_key, SP_CS, strlen(SP_CS))==0){
		ch_no = SPI_CS;
	}else{
		return(rtn);
	}

	if(semaphore_lock(SEMAPHORE_LOCK)==0){
		return(rtn);
    }

	ch_cnt = (CHANNEL_CNT *)(G_SHM_ADR + 1);
	ch_info = (CHANNEL_INFO *)(ch_cnt + 4);
	ch_cnt += ch_no;
	ch_info += ch_cnt->offset;
	for(uint32_t i=0; i < ch_cnt->cnt; i++, ch_info++){
		if(strcmp(ch_info->channel_key, ch_key)==0){
			if(i+cnt < ch_cnt->cnt){ 
				if(strcmp((ch_info+cnt)->channel_key, ch_key)==0){
					memcpy(output, (ch_info+cnt), sizeof(CHANNEL_INFO));
					rtn = CH_RETURN_FOUND;
				}
			}
			break;
		}
	}
	semaphore_lock(SEMAPHORE_UNLOCK);
	return(rtn);
}


int search_tuning_space(uint8_t tuning_space, uint32_t cnt, CHANNEL_INFO *output)
{
	int rtn = CH_RETURN_NOTFOUND;
	CHANNEL_CNT *ch_cnt;
	CHANNEL_INFO *ch_info;

	if(!(tuning_space==SPI_UHF || tuning_space==SPI_CATV || tuning_space==SPI_BS || tuning_space==SPI_CS)){
		return(CH_RETURN_NOTFOUND);
	}

	if(semaphore_lock(SEMAPHORE_LOCK)==0){
		return(rtn);
    }

	ch_cnt = (CHANNEL_CNT *)(G_SHM_ADR + 1);
	ch_info = (CHANNEL_INFO *)(ch_cnt + 4);
	ch_cnt += tuning_space;
	ch_info += ch_cnt->offset;

	if(cnt >= 0  && cnt < ch_cnt->cnt){
		memcpy(output, (ch_info+cnt), sizeof(CHANNEL_INFO));
		rtn = CH_RETURN_FOUND;
	}

	semaphore_lock(SEMAPHORE_UNLOCK);
	return(rtn);
}



int search_ch_sid(char *ch_key, uint32_t sid, CHANNEL_INFO *output)
{
	int ret = CH_RETURN_NOTFOUND;

	for(uint32_t i=0; ;i++){
		if(search_channel_key(ch_key, i, output)==CH_RETURN_FOUND){
			if(output->sid == sid){
				ret = CH_RETURN_FOUND;
				break;
			}
		}else{
			break;
		}
	}
	return(ret);
}

int search_ts_sid(uint32_t tsid, uint32_t sid, CHANNEL_INFO *output)
{
	int ret = CH_RETURN_NOTFOUND;

	for(uint32_t i=0; ;i++){
		if(search_tsid(tsid, i, output)==CH_RETURN_FOUND){
			if(output->sid == sid){
				ret = CH_RETURN_FOUND;
				break;
			}
		}else{
			break;
		}
	}
	return(ret);
}

void __attribute__ ((constructor)) channel_cnf_init_proc(void)
{
	size_t shm_size;

	int ret = ipc_init();
	if(ret==0){
		exit(0);
	}

	ret = semaphore_init();
	if(ret < 0){
		exit(0);
	}

	ret = shared_memory_init(&shm_size);
	if(ret < 0){
		exit(0);
	}
}


void __attribute__ ((destructor)) channel_cnf_end_proc(void)
{
	if(G_SHM_ADR){
		shmdt(G_SHM_ADR);
	}
}


