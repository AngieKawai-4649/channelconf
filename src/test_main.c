
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "channel_cnf.h"

int main()
{
	CHANNEL_INFO w;


/**
	printf("sleep(60)\n");
	sleep(20);
	printf("wake up\n");
**/

	printf("地デジ\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_UHF, i, &w) == CH_RETURN_FOUND){
			print_channel_info(stdout, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}

	printf("CATV\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_CATV, i, &w) == CH_RETURN_FOUND){
			print_channel_info(stdout, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}
	printf("BS\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_BS, i, &w) == CH_RETURN_FOUND){
			print_channel_info(stdout, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}
	printf("CS\n");
	for(uint8_t i = 0; ; i++){
		if(search_tuning_space(SPI_CS, i, &w) == CH_RETURN_FOUND){
			print_channel_info(stdout, &w);
		}else{
			printf("search_tuning_space() NOT FOUND [%u]\n", i);
			break;
		}
	}

	char ch_key[256];
	char buf[256];
	uint32_t sid, tsid;
	CHANNEL_INFO ch_info;

	printf("input ch_key\n");
	int ret = scanf("%255s", ch_key);
	printf("input tsid\n");
	ret = scanf("%255s", buf);
	tsid = strtoul(buf, NULL, 0);
	printf("input sid\n");
	ret = scanf("%255s", buf);
	sid = strtoul(buf, NULL, 0);

	for(uint32_t i=0; ;i++){
		ret = search_channel_key(ch_key, i, &ch_info);
		printf("search_channel_key() return %d\n", ret);
		if(ret==CH_RETURN_FOUND){
			print_channel_info(stdout, &ch_info);
		}else{
			printf("NOT FOUND\n");
			break;
		}
	}

	for(uint32_t i=0; ;i++){
	ret = search_tsid(tsid, i, &ch_info);
		printf("seach_tsid() return %d\n", ret);
		if(ret==CH_RETURN_FOUND){
			print_channel_info(stdout, &ch_info);
		}else{
			printf("NOT FOUND\n");
			break;
		}
	}

	ret = search_ch_sid(ch_key, sid, &ch_info);
	printf("seach_ch_sid() return %d\n", ret);
	if(ret==CH_RETURN_FOUND){
		print_channel_info(stdout, &ch_info);
	}else{
		printf("NOT FOUND\n");
	}

	ret = search_ts_sid(tsid, sid, &ch_info);
	printf("seach_ts_sid() return %d\n", ret);
	if(ret==CH_RETURN_FOUND){
		print_channel_info(stdout, &ch_info);
	}else{
		printf("NOT FOUND\n");
	}
}



