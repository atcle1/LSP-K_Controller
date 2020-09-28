#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>		//close()
#include <sys/time.h>
#include <sys/ioctl.h> 		//_IO,_IOR,_IOW
#include <linux/rtc.h> 
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "spr_parse.h"

// [4byte:index][1byte:type] {packet}

#define PACKET_TEXT_WT			0
/* [timespec(8)][messge][\0] */
#define PACKET_TEXT				1
/* [messge][\0] */
#define PACKET_BINARY_WT		2
/* [timespec(8)][subtype(1)][size(1)][binary(max:256)] */
#define PACKET_BINARY			3
/* [subtype(1)][size(1)][binary(max:256)] */
#define PACKET_BIG_BINARY_WT	4
/* [timespec(8)][subtype(1)][size(2)][binary(max:64KB)] */
#define PACKET_BIG_BINARY		5
/* [subtype(1)][size(2)][binary(max:64KB)] */

int init_textlog_itr(char *log, struct spr_log_itr *itr)
{
	struct packet_header_impl *packet_header;
	itr->start_buf = itr->pnow = log;
	packet_header = (struct packet_header_impl*)log;
	itr->idx = packet_header->idx;
	itr->type = packet_header->type;

	if (itr->type <0 || itr->type >10) {
		return -1;
	}
	itr->size = get_packet_size_(itr->pnow);

	return itr->type;
}

int inspect(char *arr)
{
	int i;
	for (i=0; i < 50; i++)
	{
		printf("%2d:%02X ", i, arr[i]& 0xFF);
		if((i+1)%10 == 0)
			printf("\n");
	}
	printf("\n");
}

int get_packet_size_(char *ppacket)
{
	int msg_len;
	char *pbody, *ptmp;
	int binary_len;
	struct packet_header_impl* packet_header;
	struct timespec *ptimespec;
	packet_header = (struct packet_header_impl*)ppacket;
	ptmp = pbody = ppacket + sizeof(struct packet_header_impl);

	switch (packet_header->type) {
		case PACKET_TEXT_WT:
			ptmp += 8;
			msg_len = strlen(pbody + 8);
			ptmp += msg_len + 1;
		break;
		case PACKET_TEXT:
			msg_len = strlen(pbody);
			ptmp += msg_len + 1;
		break;
		case PACKET_BINARY:
			ptmp++;	// subtype
			binary_len = *(unsigned char*)ptmp;
			ptmp += 1;	// subtype, size
			ptmp += binary_len;
		break;
		case PACKET_BINARY_WT:
			ptmp += 8;	// time
			ptmp += 1;	// subtype
			binary_len = *pbody;
			ptmp += 1;	// size
			ptmp += binary_len;
		break;
		case PACKET_BIG_BINARY:
			ptmp++;	// subtype
			binary_len = *(unsigned short*)ptmp;
			ptmp += 2;	// subtype, size
			ptmp += binary_len;
		break;
		case PACKET_BIG_BINARY_WT:
			ptmp += 8;	// time
			ptmp += 1;	// subtype
			binary_len = *pbody;
			ptmp += 2;	// size
			ptmp += binary_len;
		break;
	}
	return ptmp - ppacket;
}

unsigned char next_log(struct spr_log_itr *itr)
{
	struct timespec *ptimespec;
	struct tm* tmm;
	time_t tm;
	int index;
	int msg_len;

	struct packet_header_impl* packet_header;

	if (itr->idx == -1/* || itr->idx>2000*/) {
		return -1;
	}

	// next...
	itr->pnow = itr->pnow + itr->size;
	packet_header = (struct packet_header_impl*)itr->pnow;
	if (packet_header->idx == -1) {
		itr->idx = -1;
		itr->type = 255;
		itr->size = 0;
		return -1;
	} else {
		itr->idx = packet_header->idx;
		itr->type = packet_header->type;
	}
	itr->size = get_packet_size_(itr->pnow);

	return packet_header->type;
}

int read_kernel_timespec(char *buf, struct timespec *timespec)
{
	char *pnow = buf;
	timespec->tv_sec = (int*)pnow;
	pnow+=4;
	timespec->tv_nsec = (int*)pnow;
	pnow+=4;

}

int test(unsigned int a)
{
	unsigned char bytes[4];
	unsigned long n = a;

	bytes[0] = (n >> 24) & 0xFF;
	bytes[1] = (n >> 16) & 0xFF;
	bytes[2] = (n >> 8) & 0xFF;
	bytes[3] = n & 0xFF;
	printf("%d : %x %x %x %x\n", a, bytes[0], bytes[1], bytes[2], bytes[3]);
}

int parse_timespec(char *buf, struct timespec *timespec)
{
	timespec->tv_sec = *(unsigned int*)buf;
	timespec->tv_nsec = *(unsigned int*)(buf + 4);
	return 8;
}

int parse_text_wt(struct spr_log_itr *itr, struct spr_textlog_item *oitem)
{
	struct timespec timespec;
	struct tm* tmm;
	time_t tm;
	int index;
	int msg_len;
	char *pnow;
	int tmp;
	unsigned int tv_sec;
	unsigned int tv_nsec;

	pnow = itr->pnow;
	oitem->idx = *(int*)pnow;


	if (oitem->idx == -1) {
		return -1;
	}
	pnow += sizeof(struct packet_header_impl);

	// timespec
	oitem->bTime = 1;
	pnow += parse_timespec(pnow, &timespec);
	
	tm = timespec.tv_sec + 3600 * 9;
	tmm = gmtime(&tm);
	//printf("%d %x %x %d\n", sizeof(struct packet_header_impl), &oitem->tmm, tmm, timespec.tv_sec);
	if (tmm != NULL)
		memcpy(&(oitem->tmm), tmm, sizeof(struct tm));
	oitem->nsec = timespec.tv_nsec;

	// msg
	msg_len = strlen(pnow);
	memcpy(oitem->msg, pnow, msg_len + 1);
	pnow += msg_len + 1;
	
	return 0;
}

int parse_text(struct spr_log_itr *itr, struct spr_textlog_item *oitem)
{
	struct timespec *ptimespec;
	struct tm* tmm;
	time_t tm;
	int index;
	int msg_len;
	char *pnow;
	pnow = itr->pnow;
	oitem->idx = *(int*)pnow;
	if (oitem->idx == -1) {
		return -1;
	}
	pnow += sizeof(struct packet_header_impl);
	oitem->bTime = 0;

	// msg
	msg_len = strlen(pnow);
	memcpy(oitem->msg, pnow, msg_len + 1);
	pnow += msg_len + 1;
	return 0;
}

int parse_binary(struct spr_log_itr *itr, struct spr_binarylog_item *oitem, int bTime)
{
	struct timespec timespec;
	struct tm* tmm;
	time_t tm;
	int index;
	int msg_len;
	unsigned char *pnow;
	int tmp;
	unsigned int tv_sec;
	unsigned int tv_nsec;

	pnow = itr->pnow;
	oitem->idx = *(int*)pnow;


	if (oitem->idx == -1) {
		return -1;
	}
	pnow += sizeof(struct packet_header_impl);

	if (bTime) {
		// timespec
		oitem->bTime = 1;
		pnow += parse_timespec(pnow, &timespec);
		
		tm = timespec.tv_sec + 3600 * 9;
		tmm = gmtime(&tm);
		//printf("%d %x %x %d\n", sizeof(struct packet_header_impl), &oitem->tmm, tmm, timespec.tv_sec);
		if (tmm != NULL)
			memcpy(&(oitem->tmm), tmm, sizeof(struct tm));
		oitem->nsec = timespec.tv_nsec;
	} else {
		oitem->bTime = 0;
	}
	// binary
	oitem->subtype = *(pnow);
	pnow += 1;

	msg_len = *(pnow);
	pnow += 1;

	oitem->size = msg_len;
	memcpy(oitem->buf, pnow, msg_len);
	pnow += msg_len;
	
	return 0;
}

int parse_big_binary(struct spr_log_itr *itr, struct spr_binarylog_item *oitem, int bTime)
{
	struct timespec timespec;
	struct tm* tmm;
	time_t tm;
	int index;
	int msg_len;
	unsigned char *pnow;
	int tmp;
	unsigned int tv_sec;
	unsigned int tv_nsec;

	pnow = itr->pnow;
	oitem->idx = *(int*)pnow;


	if (oitem->idx == -1) {
		return -1;
	}
	pnow += sizeof(struct packet_header_impl);

	if (bTime) {
		// timespec
		oitem->bTime = 1;
		pnow += parse_timespec(pnow, &timespec);
		
		tm = timespec.tv_sec + 3600 * 9;
		tmm = gmtime(&tm);
		//printf("%d %x %x %d\n", sizeof(struct packet_header_impl), &oitem->tmm, tmm, timespec.tv_sec);
		if (tmm != NULL)
			memcpy(&(oitem->tmm), tmm, sizeof(struct tm));
		oitem->nsec = timespec.tv_nsec;
	} else {
		oitem->bTime = 0;
	}

	// binary
	oitem->subtype = *(pnow);
	pnow += 1;

	msg_len = *(unsigned short*)pnow;
	pnow += 2;

	oitem->size = msg_len;
	memcpy(oitem->buf, pnow, msg_len);
	pnow += msg_len;
	
	return 0;
}

void print_textlog_item(struct spr_textlog_item *item)
{
	struct tm* tmm = &item->tmm;
	if (item->bTime) {
		printf("%d-%02d-%02d %02d:%02d:%02d.%09lu | ",
			tmm->tm_year + 1900, tmm->tm_mon + 1, tmm->tm_mday,
			tmm->tm_hour, tmm->tm_min, tmm->tm_sec, item->nsec);
	} else
		printf("                              | ");
	printf("%5d | %s\n", item->idx, item->msg);
}

void print_cpufreq(unsigned char *buf, int size)
{
	int i = 0;
	unsigned int pstate;
	unsigned int executedTime;
	printf("CPUSTAT : ");
	while (i < size)
	{
		pstate = buf[i];
		i++;
		executedTime = *(int*)(buf+i);
		i += 4;
		printf("[%d:%d] ", pstate, executedTime);
	}
	putchar('\n');
}
void print_binarylog_item(struct spr_binarylog_item *item)
{
	struct tm* tmm = &item->tmm;
	if (item->bTime) {
		printf("%d-%02d-%02d %02d:%02d:%02d.%09lu\t",
			tmm->tm_year + 1900, tmm->tm_mon + 1, tmm->tm_mday,
			tmm->tm_hour, tmm->tm_min, tmm->tm_sec, item->nsec);
	} else
		printf("                              | ");
	printf("%5d | ", item->idx);
	switch (item->subtype){
		case 3:
			print_cpufreq(item->buf, item->size);
		break;
		default:
			printf("unknown subtype : %d, size : %d\n", item->subtype, item->size);
		break;
	}
}
int read_all_log(char *log)
{
	struct spr_log_itr itr;
	int type = -1;
	struct spr_textlog_item log_text_wt;
	struct spr_binarylog_item log_binary_item;
	struct spr_big_binarylog_item log_big_binary_item;
	//printf("read all log\n");
	//read_all_log(log);
	type = init_textlog_itr(log, &itr);
	do {
		switch (type) {
			case PACKET_TEXT_WT:
				parse_text_wt(&itr, &log_text_wt);
				print_textlog_item(&log_text_wt);
			break;
			case PACKET_TEXT:
				parse_text(&itr, &log_text_wt);
				print_textlog_item(&log_text_wt);
			break;
			case PACKET_BINARY_WT:
				//printf("PACKET_BINARY_WT\n");
				parse_binary(&itr, &log_binary_item, 1);
				print_binarylog_item(&log_big_binary_item);
			break;
			case PACKET_BINARY:
				//printf("PACKET_BINARY\n");
				parse_binary(&itr, &log_binary_item, 0);
				print_binarylog_item(&log_big_binary_item);
			break;
			case PACKET_BIG_BINARY_WT:
				//printf("PACKET_BINARY_WT\n");
				parse_big_binary(&itr, &log_big_binary_item, 1);
				print_binarylog_item(&log_big_binary_item);
			break;
			case PACKET_BIG_BINARY:
				//printf("PACKET_BIG_BINARY\n");
				parse_big_binary(&itr, &log_big_binary_item, 0);
				print_binarylog_item(&log_big_binary_item);
			break;
			default :
				printf("unknown %d\n", type);
			break;
		}
	} while ((type = next_log(&itr)) != 255);
}
