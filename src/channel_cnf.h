#ifndef CHANNEL_CNF
#define CHANNEL_CNF
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define IPC_DEFAULT_KEY 0x4649
#define CHANNEL_IPC_KEY		"CHANNELCONFKEY"	// 環境変数 CHANNELCONFKEY
#define CHANNEL_FILE_PATH	"CHANNELCONFPATH"	// 環境変数 CHANNELCONFPATH
#define CHANNEL_FILE "channel.cnf"
#define SP_UHF  "UHF"
#define SP_CATV "CATV"
#define SP_BS   "BS"
#define SP_CS   "CS"
#define SPI_UHF  0
#define SPI_CATV 1
#define SPI_BS   2
#define SPI_CS   3

#define SEMAPHORE_LOCK   1
#define SEMAPHORE_UNLOCK 0

#define CH_RETURN_FOUND     1
#define CH_RETURN_NOTFOUND  0

#define CH_LOG_OFF			0
#define CH_LOG_STDOUT		1
#define CH_LOG_STDERR		2


typedef struct {
	uint32_t  cnt;
	uint32_t  offset;
} CHANNEL_CNT; // [0]:UHF [1]:CATV, [2]:BS, [3]:CS

typedef struct {
	uint8_t  tuning_space;		// UHF 0, CATV 1, BS 2, CS 3
	char     channel_key[16];	// チャンネルキー Mirakurunと一致させる
	char     channel[4];		// UHF 13-62 CATV C13-C63 BS 1-23 CS 2-23
	uint8_t  l_tsid;			// 論理TSID BS 0-6  BS以外は0
	uint8_t  pt_ch;				// PT1-3 Channel
	uint32_t freq;				// frequency Mhz * 1000   1471.44MHz -> 14714400
	uint32_t sid;				// Service ID
	uint32_t tsid;				// TS ID
	uint8_t  view;				// 視聴可否 視聴する:1  視聴しない:0
								// 1: playlist.m3u8に出力する
								// 0: playlist.m3u8に出力しない
	uint8_t  mirakurun;			// Mirakurun channels.yml出力可否
								//  3: isDisabled: false 有料チャンネルを視聴する
								//  2: isDisabled: true  有料チャンネルを視聴視聴しない
								//  1: 無料放送 isDisabled: を出力しない
								//  0: チャンネル情報を Mirakurun channels.ymlに出力しない
	char     ch_name[256];

} CHANNEL_INFO;

union semun {
	int val;
	struct semid_ds *buf;
	uint16_t *array;
};

extern int search_ts_sid(uint32_t tsid, uint32_t sid, CHANNEL_INFO *output);
extern int search_ch_sid(char *ch_key, uint32_t sid, CHANNEL_INFO *output);
extern int search_tsid(uint32_t tsid, uint32_t cnt, CHANNEL_INFO *output);
extern int search_tuning_space(uint8_t tuning_space, uint32_t cnt, CHANNEL_INFO *output);
extern int search_channel_key(char *ch_key, uint32_t cnt, CHANNEL_INFO *output);

extern int shared_memory_init(size_t *cs);
extern int read_ch_cnt(CHANNEL_CNT *ch_cnt);
extern int read_ch_info();
extern int shm_delete();
extern int get_ch_cnt(CHANNEL_CNT *ch_cnt);
extern int set_ch_cnt(CHANNEL_CNT *ch_cnt);
extern void print_channel_cnt(FILE *fp);
extern void print_channel_info(FILE *fp, CHANNEL_INFO *ch_info);

#define MSG_OUTPUT(out, ...) { \
							switch(out) { \
							case CH_LOG_OFF: \
								break; \
							case CH_LOG_STDOUT: \
								fprintf(stdout, __VA_ARGS__); \
								break; \
							case CH_LOG_STDERR: \
								fprintf(stderr, __VA_ARGS__); \
								break; \
							} \
						}

#endif
