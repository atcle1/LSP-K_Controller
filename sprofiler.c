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

#define DEV_MAJOR_NO            10

typedef unsigned	u32;
#define WRITE_START_MONITORING		_IOW(DEV_MAJOR_NO,0,u32)
#define WRITE_STOP_MONITORING		_IOW(DEV_MAJOR_NO,1,u32)

#define WRITE_PAUSE_MONITORING	_IOW(DEV_MAJOR_NO,2,u32)
#define WRITE_RESUME_MONITORING	_IOW(DEV_MAJOR_NO,3,u32)

#define READ_LOG_BUF_STATUS		_IOR(DEV_MAJOR_NO,4,u32)

#define READ_PROFILE_RESULT		_IOWR(DEV_MAJOR_NO,5,u32)
#define READ_PROFILE_LOG		_IOWR(DEV_MAJOR_NO,6,u32)

#define READ_PROFILER_STATUS		_IOWR(DEV_MAJOR_NO,7,u32)
#define WRITE_PROFILER_BUF_CLEAR		_IOWR(DEV_MAJOR_NO,8,u32)
#define WRITE_PROFILER_LOG		_IOWR(DEV_MAJOR_NO,9,u32)
#define READ_SESSION_STATUS		_IOWR(DEV_MAJOR_NO,10,u32)

//
#define SPR_STATUS_UNKNOWN	0
#define SPR_STATUS_INITED	1	// initialized in booting
#define SPR_STATUS_STARTED	2	// when ready to logging, automatically go to next state...
#define SPR_STATUS_RESUMMED	3	// now logging
#define SPR_STATUS_PAUSED	4	// now paused, could go to resummed state anytime.
#define SPR_STATUS_REBOOT	5	// spr is paused for shutdown, (but automatically resumed when next booting end.)
#define SPR_STATUS_STOPED	6	// stop logging, do not restore the state next booting time.
//
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
int status  = 0;
int dev;
char *mmap_page;

int start_monitoring()
{
	/* Implement here */
	return ioctl(dev, WRITE_START_MONITORING);
}

int stop_monitoring()
{
	/* Implement here */
	ioctl(dev, WRITE_STOP_MONITORING);
	return close(dev);
}
/*
int get_sprsession_log(struct spr_session_log *info_buf, int buf_size, int *rval)
{
	unsigned int argarr[3];
	argarr[0] = (unsigned int)info_buf;
	argarr[1] = (unsigned int)buf_size;
	argarr[2] = (unsigned int)rval;
	return ioctl(dev, READ_PROFILE_RESULT, argarr);
}



int get_sprtext_log(char *logbuf, int maxsize, int *rcnt)
{
	unsigned int argarr[3];
	argarr[0] = (unsigned int)logbuf;
	argarr[1] = (unsigned int)maxsize;
	argarr[2] = (unsigned int)rcnt;
	return ioctl(dev, READ_PROFILE_LOG, argarr);
}
*/

int write_log_file_(char *dirpath, char *filename, char *log, unsigned int logsize)
{
	int fd;
	char path[50];
	char txtbuf[30];
	int written, total_written = 0, write_size;
	sprintf(path, "%s/%s", dirpath, filename);
	printf("%s\n", path);
	fd = creat(path, 0644);

	if (fd < 0) {
		printf("creat error\n");
		return -1;
	}

	sprintf(txtbuf, "MDProLog\nLOGBEGIN\n");
	write(fd, txtbuf, strlen(txtbuf));


	while(total_written < logsize) {
		write_size = logsize - total_written < 65535 ? logsize - total_written : 65535;
		written = write(fd, log + total_written, write_size);
		if (written < 0) {
			fprintf(stderr, "write error  logsize:%d fd:%d write_size:%d total_written:%d\n", logsize, fd, write_size, total_written);
			close(fd);
			return -1;
		}
		total_written += written;
	}

	close(fd);
	return 0;
}


int read_log_file(char *path)
{
	FILE *fp;
	int total_read = 0;
	static char buf[4*1024*1024];	//1MB
	if (path == NULL)
		fp = fopen("/sdcard/LSP/noname.klog", "r");
	else
		fp = fopen(path, "r");
	if (fp == NULL) {
		printf("fopen error\b\n");
		return -1;
	}
	while(fgets(buf, 50, fp) != NULL) {
		printf("%s", buf);
		if (strcmp(buf, "LOGBEGIN\n") == 0) {
			printf("***logstart\n");
			break;
		}
	}
	total_read = fread(buf, sizeof(char), sizeof(buf), fp);
	printf("total read : %d\n", total_read);
	*(int*)&buf[total_read] = -1;
	read_all_log(buf);
	fclose(fp);
}

int write_log_file(const char *pathDir, const char *fileName)
{
	int bResummed = 0;
	unsigned int argarr[3];
	int rval;
	
	if (status == SPR_STATUS_RESUMMED) {
		ioctl(dev, WRITE_PAUSE_MONITORING, NULL);
		bResummed = 1;
	}
	rval = ioctl(dev, READ_LOG_BUF_STATUS, argarr);
	if (rval < 0)
		printf("err log size : %d %d %d, rval : %d\n", argarr[0], argarr[1], argarr[2], rval);
	else {
		printf("log size : %d %d %d\n", argarr[0], argarr[1], argarr[2]);
		write_log_file_(pathDir, fileName, mmap_page, argarr[0]);
	}

	if (bResummed == 1) {
		printf("resume logfile\n");
		ioctl(dev, WRITE_RESUME_MONITORING, NULL);
	}
	return 0;
}

int write_log_file_and_clear_buf(const char *pathDir, const char *fileName)
{
	int bResummed = 0;
	unsigned int argarr[3];
	int rval;
	
	if (status == SPR_STATUS_RESUMMED) {
		ioctl(dev, WRITE_PAUSE_MONITORING, NULL);
		bResummed = 1;
	}
	rval = ioctl(dev, READ_LOG_BUF_STATUS, argarr);
	if (rval < 0)
		printf("err log size : %d %d %d, rval : %d\n", argarr[0], argarr[1], argarr[2], rval);
	else {
		printf("log size : %d %d %d\n", argarr[0], argarr[1], argarr[2]);
		write_log_file_(pathDir, fileName, mmap_page, argarr[0]);
	}

	clear_log_buf();

	if (bResummed == 1) {
		printf("resume logfile\n");
		ioctl(dev, WRITE_RESUME_MONITORING, NULL);
	}
	return 0;
}


int touch_log_finish_file(const char *dirpath, const char *filename)
{
	int fd;
	char path[50], txtbuf[50];
	sprintf(path, "%s/%s.finish", dirpath, filename);
	fd = creat(path, 0644);
	if (fd > -1) {
		sprintf(txtbuf, "write_end\n");
		write(fd, txtbuf, strlen(txtbuf));
		close(fd);
	} else
		printf("touch log finish file open error");
	return 0;
}


int show_log_buf_status()
{
	int argarr[3];
	ioctl(dev, READ_LOG_BUF_STATUS, argarr);
	printf("log size : %u %u %u\n", argarr[0], argarr[1], argarr[2]);
	return 0;
}

void clear_log_buf()
{
	int argarr[3];
	printf("logbuf clear\n");
	ioctl(dev, WRITE_PROFILER_BUF_CLEAR, NULL);
	return;
}

int write_spr_log(const char *log)
{
	return ioctl(dev, WRITE_PROFILER_LOG, log);
}

int get_sprtext_mmap_log(char *logbuf, int maxsize, int *rcnt)
{
	unsigned int argarr[3];
	// ioctl(dev, WRITE_PAUSE_MONITORING, NULL);
	ioctl(dev, READ_LOG_BUF_STATUS, argarr);
	printf("log size : %d %d %d\n", argarr[0], argarr[1], argarr[2]);
	read_all_log(logbuf);
	//ioctl(dev, WRITE_RESUME_MONITORING, NULL);
	return 0;
}

int get_spr_status(int *status)
{
	int rval, argarr[3];
	rval = ioctl(dev, READ_PROFILER_STATUS, argarr);
	*status = argarr[0];
	return rval;
}

int init_debugfs()
{
	int configfd;
	char buf[20];
	configfd = open("/sys/kernel/debug/spr", O_RDWR);
	//configfd = open("/dev/misc_spr",O_RDWR);
	if(configfd < 0) {
		printf("open error\n");
		return configfd;
	}

	mmap_page = mmap(NULL, PAGE_SIZE*1024, PROT_READ, MAP_SHARED, configfd, 0);
	if (mmap_page == MAP_FAILED) {
		printf("mmap error\n");
		return -1;
	}
	return configfd;
}

int mkdir_recursively(char *dir)
{
        char tmp[256];
        char *p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, 0755);
                        *p = '/';
                }
        mkdir(tmp, 0755);
}

void backup_klog_condition(const char *pathDir, const char *fileName)
{
	int argarr[3];
	char textbuf[30];
	float leftP;
	int debug = 0;

	ioctl(dev, READ_LOG_BUF_STATUS, argarr);
	 leftP = (float)argarr[1]/argarr[2];
	 printf("buffer left size : %d/%d = %f\n", argarr[1], argarr[2], leftP);
	if (leftP < 0.4 || debug) {
		printf("try klog backup, buffer left size : %d/%d = %f\n", argarr[1], argarr[2], leftP);
	} else {
		return;
	}

	ioctl(dev, READ_SESSION_STATUS, argarr);
	// touch cnt, brightness, last wake term
	printf("status : touch_cnt : %u brightness : %u last_wake_term : %u\n", argarr[0], argarr[1], argarr[2]);


	if ((argarr[0] == 0 && argarr[1] == 0 && argarr[2] < 10) || leftP < 0.2 || debug) {
		printf("klog backup condition\n");
	} else {
		sprintf(textbuf, "BUF isn't enough(%.3f), can't back up! %d %d %d\n", leftP, argarr[0], argarr[1], argarr[2]);
		write_spr_log(textbuf);
		return;
	}
	write_log_file_and_clear_buf(pathDir, fileName);
	touch_log_finish_file(pathDir, fileName);

}

void write_spr_log_test(int cnt)
{
	int i = 0;
	char buf[20];
	for (i = 0; i < cnt; i++) {
		sprintf(buf, "TEST%5d", cnt);
		write_spr_log(buf);
	}
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

	//execlp( "echo", "echo", "test", ">",  "/dev/kmsg", 0);
	dev = init_debugfs();
	if (dev < 0) {
		printf("init_debugfs error\n");
		return -1;
	}
	rval = get_spr_status(&status);
	mkdir_recursively("/sdcard/LSP");
	if (argc == 1)
	{
		printf("cmd : \n0 start server\n1 start_monitoring\n2 stop_monitoring\n3 write_log_file\n4 get status\n5 read_log_file\n6 read_current_log\n7 clear_log\n8 write_log\n9 try klogbackup\n");
		printf("enter command : ");
		scanf("%d", &cmd);	
	} else if (argc > 1) {
		cmd = atoi(argv[1]);
		for (i = 0; i < argc; i++) {
			printf("%d : %s\n", i, argv[i]);
		}
	}

	printf("input cmd : %d\n", cmd);

	if (cmd == 0) {
		bServerStarted = 1;
		start_server(10001);
	} else if (cmd == 1){
		start_monitoring();
	} else if (cmd == 2) {
		stop_monitoring();
	} else if (cmd == 3) {
		printf("write log file\n");
		if (argc == 3)
			write_log_file(argv[2], "noname.klog");
		else if (argc == 4) {
			write_log_file(argv[2], argv[3]);
			touch_log_finish_file(argv[2], argv[3]);
		} else //2
			write_log_file("/sdcard/LSP", "noname.klog");
	} else if (cmd == 4) {
		printf("status : %d\n", status);
		show_log_buf_status();
		if (rval == 0)
			return status;
	} else if (cmd == 5) {
		show_log_buf_status();
		if (argc == 3)
			read_log_file(argv[2]);
		else
			read_log_file(NULL);
	} else if (cmd == 6) {
		get_sprtext_mmap_log(mmap_page, PAGE_SIZE*1024, &cnt);
	} else if (cmd == 7) {
		clear_log_buf();
	} else if (cmd == 8) {
		write_spr_log(argv[2]);
	} else if (cmd == 9) {
		if (status != 3) {
			printf("MDPro-K status : %d\nNot resumed\n", status);
			return;
		}
		if (argc == 3)
			backup_klog_condition("/sdcard/LSP", argv[2]);
		else if (argc == 4) {
			backup_klog_condition(argv[2], argv[3]);
		} else //2
			backup_klog_condition("/sdcard/LSP", "noname.klog");
	} else if (cmd == 10) {
		if (argc == 2)
			write_spr_log_test(10000);
		else
			write_spr_log_test(atoi(argv[2]));
	} else {
		printf("unknown cmd\n");
	}

	if (bServerStarted == 0)
		join_server_thread();

	return 0;
}
