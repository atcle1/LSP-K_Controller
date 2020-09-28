#ifndef __SPR_PARSE_H
#define __SPR_PARSE_H

struct spr_session_log {
	int goto_suspend_state;
	int wakeup_error;
	struct timespec s_ts, e_ts;
};

struct spr_log_itr {
	char *start_buf;
	char *pnow;
	int idx;
	int type;
	int size;
};

struct packet_header_impl {
	int idx;
	unsigned char type;
} __attribute__((packed));

struct spr_textlog_item {
	int pos;
	int idx;
	int bTime;
	struct tm tmm;
	unsigned long nsec;
	char msg[129];
};

struct spr_binarylog_item {
	int pos;
	int idx;
	int bTime;
	struct tm tmm;
	unsigned long nsec;

	int subtype;
	int size;
	char buf[256];
};

struct spr_big_binarylog_item {
	int pos;
	int idx;
	int bTime;
	struct tm tmm;
	unsigned long nsec;

	int subtype;
	int size;
	char buf[512];	// TODO : dynamic allocating...
};

#endif
