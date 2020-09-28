#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>		//close()
#include <sys/time.h>
#include <linux/rtc.h> 
#include <time.h>
#include <string.h>

#include "spr_parse.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
int status  = 0;
int dev;
char *mmap_page;


int read_log_file(char *path)
{
	FILE *fp;
	int total_read = 0;
	static char buf[4*1024*1024];	//1MB
	if (path == NULL)
		fp = fopen("/sdcard/LSP/log.dat", "r");
	else
		fp = fopen(path, "r");
	if (fp == NULL) {
		printf("fopen error\b\n");
		return -1;
	}
	while(fgets(buf, 50, fp) != NULL) {
		printf("%s", buf);
		if (strcmp(buf, "LOGBEGIN\n") == 0) {
			//printf("***logstart\n");
			break;
		}
	}
	total_read = fread(buf, sizeof(char), sizeof(buf), fp);
	//printf("total read : %d\n", total_read);
	*(int*)&buf[total_read] = -1;
	read_all_log(buf);
	fclose(fp);
}

/* main function */
int main(int argc, char *argv[])
{

	int cmd = 0;
	int rval = 0;
	int cnt = 0;
	int i;

	int bServerStarted = 0;
	char buf[10];

	struct spr_session_log session_log[1000];
	struct spr_log_itr itr;
	struct spr_textlog_item logitem;

	if (argc == 2)
		read_log_file(argv[1]);
	else
		printf("usage : %s <logfile>\n", argv[0]);

	return 0;
}
