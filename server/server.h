#ifdef	SPR_DAEMON_SERVER_H
#define	SPR_DAEMON_SERFER_H

extern int debug_mode;

int start_server(int port);
int join_server_thread();
int is_exited_server();


#endif