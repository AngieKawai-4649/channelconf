
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "channel_cnf.h"

void print_warn()
{
	fprintf(stderr, "ch_tool [option]\n");
	fprintf(stderr, "-c : channel.cnfを読み込み共有メモリを割り付けデータを展開する\n");
	fprintf(stderr, "     既に割り付けられていてchannel.cnfの内容とサイズが異なる場合\n");
	fprintf(stderr, "     削除して再割り当てする\n");
	fprintf(stderr, "-d : 共有メモリを削除する\n");
	fprintf(stderr, "-m : 共有メモリからMirakurun channels.yml フォーマットで出力する\n");
	fprintf(stderr, "-p : 共有メモリからVLC(SMPLAYER)プレイリストフォーマットで出力する\n");
	fprintf(stderr, "-v : 共有メモリ内容を表示する\n");
	fprintf(stderr, "-h : 使い方を表示する\n");
}


static int shm_load()
{
	int ret;
	size_t sz, max;
	CHANNEL_CNT ch_cnt[4];

	if(read_ch_cnt(ch_cnt)!=1){
		fprintf(stderr, "channel.cnf 読み込み失敗\n");
		return(-1);
	}

	max = sizeof(uint32_t) + sizeof(CHANNEL_CNT) * 4 + (ch_cnt[0].cnt + ch_cnt[1].cnt + ch_cnt[2].cnt + ch_cnt[3].cnt) * sizeof(CHANNEL_INFO);

	ret = shared_memory_init(&sz);
	// ret : 1 新規に割り当ててデータ展開済
	// ret : 0 既に割り当てられている
	if(ret==0){
		if(sz == max){
			fprintf(stderr, "共有メモリに割当済のsize %zd  新規size %zd ファイルから展開し直す\n", sz, max);
			if(set_ch_cnt(ch_cnt)){
				if(!read_ch_info()){
					ret = -1;
				}
			}else{
				ret = -1;
			}
		}else{
			fprintf(stderr, "共有メモリに割当済のsize %zd  新規size %zd 共有メモリ削除して割当しなおす\n", sz, max);
			ret = shm_delete();
			if(ret == 0){
				ret = -1;
			}
			ret = shared_memory_init(&sz);
			if(ret == 1){
				fprintf(stderr, "新規割当成功 %zd\n", sz);
			}
		}
	}

	return(ret);
}

static void print_mirakurun(FILE *fp, CHANNEL_INFO *ch_info)
{
	if(ch_info->mirakurun != 0){
		fprintf(fp, "- name: %s\n", ch_info->ch_name);
		switch(ch_info->tuning_space){
		case SPI_UHF:
		case SPI_CATV:
			fprintf(fp, "  type: GR\n");
			//fprintf(fp, "  channel: \'%s\'\n\n", ch_info->channel);
			fprintf(fp, "  channel: %s\n", ch_info->channel_key);
			fprintf(fp, "\n");
			break;
		case SPI_BS:
			fprintf(fp, "  type: BS\n");
			fprintf(fp, "  channel: %s\n", ch_info->channel_key);
			fprintf(fp, "  serviceId: %u\n", ch_info->sid);
			if(ch_info->mirakurun == 3){
				fprintf(fp, "  isDisabled: false\n");
			}else if(ch_info->mirakurun == 2){
				fprintf(fp, "  isDisabled: true\n");
			}
			fprintf(fp, "\n");
			break;
		case SPI_CS:
			fprintf(fp, "  type: CS\n");
			fprintf(fp, "  channel: %s\n", ch_info->channel_key);
			fprintf(fp, "  serviceId: %u\n", ch_info->sid);
			if(ch_info->mirakurun == 3){
				fprintf(fp, "  isDisabled: false\n");
			}else if(ch_info->mirakurun == 2){
				fprintf(fp, "  isDisabled: true\n");
			}
			fprintf(fp, "\n");
			break;
		default:
			fprintf(fp, "  FORMAT ERROR\n");
			break;
		}
	}
}

static void shm_mirakurun(FILE *fp)
{
	CHANNEL_INFO w;

	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_UHF, i, &w) == CH_RETURN_FOUND){
			print_mirakurun(fp, &w);
		}else{
			break;
		}
	}

	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_CATV, i, &w) == CH_RETURN_FOUND){
			print_mirakurun(fp, &w);
		}else{
			break;
		}
	}

	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_BS, i, &w) == CH_RETURN_FOUND){
			print_mirakurun(fp, &w);
		}else{
			break;
		}
	}

	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_CS, i, &w) == CH_RETURN_FOUND){
			print_mirakurun(fp, &w);
		}else{
			break;
		}
	}

	return;
}

static int cmp_asc(const void *x, const void *y )
{
	CHANNEL_INFO *a = *(CHANNEL_INFO **)x;
	CHANNEL_INFO *b = *(CHANNEL_INFO **)y;

	if(a->sid > b->sid){
		return(1);
	}else if(a->sid < b->sid){
		return(-1);
	}else{
		return(0);
	}
}

static void print_playlist(FILE *fp, uint8_t sp, uint32_t cnt, CHANNEL_INFO **arr, CHANNEL_INFO *data)
{
	uint32_t i = 0, output = 0;

	for(i = 0, output=0; i < cnt ; i++){
		if(search_tuning_space(sp, i, data+i) == CH_RETURN_FOUND){
			if((data+i)->view != 0){
				*(arr + output) = (data+i);
				output++;
			}
		}else{
			break;
		}
	}

	qsort(arr, output, sizeof(CHANNEL_INFO *), cmp_asc);
	for(i = 0; i < output; i++){
		fprintf(fp, "#EXTINF:-1, %s\n", (*(arr+i))->ch_name);
		switch(sp){
		case SPI_UHF:
		case SPI_CATV:
			fprintf(fp, "http://localhost:40772/api/channels/GR/%s/services/%u/stream\n", (*(arr+i))->channel_key, (*(arr+i))->sid);
			break;
		case SPI_BS:
			fprintf(fp, "http://localhost:40772/api/channels/BS/%s/services/%u/stream\n", (*(arr+i))->channel_key, (*(arr+i))->sid);
			break;
		case SPI_CS:
			fprintf(fp, "http://localhost:40772/api/channels/CS/%s/services/%u/stream\n", (*(arr+i))->channel_key, (*(arr+i))->sid);
			break;
		default:
			fprintf(fp, "  FORMAT ERROR\n");
			break;
		}
	}
}

static int shm_playlist(FILE *fp)
{
	CHANNEL_CNT		ch_cnt[4];
	CHANNEL_INFO	**ch_arr, *ch_info;

	int ret = get_ch_cnt(ch_cnt);
	if(ret){
		uint32_t max = ch_cnt[0].cnt;
		for(uint32_t i = 0; i < 4; i++){
			if(ch_cnt[i].cnt > max){
				max = ch_cnt[i].cnt;
			}
		}
		ch_info = (CHANNEL_INFO *)calloc(max, sizeof(CHANNEL_INFO));
		if(ch_info == NULL){
			ret = 0;
		}
		ch_arr = (CHANNEL_INFO **)calloc(max, sizeof(CHANNEL_INFO *));
		if(ch_arr == NULL){
			ret = 0;
		}
	
		print_playlist(fp, SPI_UHF, ch_cnt[0].cnt, ch_arr, ch_info);
		print_playlist(fp, SPI_CATV, ch_cnt[1].cnt, ch_arr, ch_info);
		print_playlist(fp, SPI_BS, ch_cnt[2].cnt, ch_arr, ch_info);
		print_playlist(fp, SPI_CS, ch_cnt[3].cnt, ch_arr, ch_info);
	
		free(ch_arr);
		free(ch_info);
	}

	return(ret);
}

static void shm_print(FILE *fp)
{
	CHANNEL_INFO w;

	print_channel_cnt(fp);

	printf("\n地デジ\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_UHF, i, &w) == CH_RETURN_FOUND){
			print_channel_info(fp, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}

	printf("CATV\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_CATV, i, &w) == CH_RETURN_FOUND){
			print_channel_info(fp, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}
	printf("BS\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_BS, i, &w) == CH_RETURN_FOUND){
			print_channel_info(fp, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}
	printf("CS\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_CS, i, &w) == CH_RETURN_FOUND){
			print_channel_info(fp, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}

	return;
}

int main(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	int ret;

	if(argc != 2){
		print_warn();
		exit(0);
	}

	while ((opt = getopt(argc, argv, "cdmpvh")) != -1) {
		switch(opt){
		case 'c':
			ret = shm_load();
			fprintf(stderr, "共有メモリ割当 %s\n", (ret>=0) ? "成功" : "失敗");
			break;
		case 'd':
			ret = shm_delete();
			fprintf(stderr, "共有メモリ削除 %s\n", (ret==1) ? "成功" : "失敗");
			break;
		case 'm':
			shm_mirakurun(stdout);
			break;
		case 'p':
			shm_playlist(stdout);
			break;
		case 'v':
			shm_print(stdout);
			break;
		case 'h':
			print_warn();
			break;
		default:
			print_warn();
			break;
		}
	}

}



