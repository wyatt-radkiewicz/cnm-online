#ifndef _MASTER_SERVER_H_
#define _MASTER_SERVER_H_

#define MSPAGE_SIZE 4
#define MAX_SERVERS 4096
#define MSTIMEOUT_PERIOD (30*15)

#define MS_SORT_COUNT 0

typedef struct _SERVER_INFO
{
	NET_ADDR addr;
	int num_players;
	int timeout_period;
	char name[32];
	char level[32];
} SERVER_INFO;
typedef struct _MSPAGE_REQUEST
{
	int page;
	int sort;
} MSPAGE_REQUEST;

typedef struct _MSPAGE_DATA
{
	int page;
	int num_pages;
	SERVER_INFO servers[MSPAGE_SIZE];
} MSPAGE_DATA;

#endif
