
#ifndef REVSOCKET_H
#define REVSOCKET_H

#define PRGVER "0.1a"

#define BUF_SIZE			1500
#define RS_MAGIC			0x6A2E

// select() timeouts
#define TV_SEC_TICK			1
#define TV_SEC_0			0
#define TV_USEC			100
#define RS_SOCKET_TO		10	// N x TV_SEC_TICK, > RS_PING_INTERVAL
#define RS_RETRY_INTERVAL	5	// N x TV_SEC_TICK
#define RS_PING_INTERVAL		5	// N x TV_SEC_TICK

// states
#define RS_ST_CONNECT		0
#define RS_ST_CLNT_WT		1
#define RS_ST_SND_LOGIN		2
#define RS_ST_WT_LOGIN		3
#define RS_ST_WT_PING		4
#define RS_ST_PING			5
#define RS_ST_WT_PONG		6
#define RS_ST_WT_MDBG		7
#define RS_ST_SND_PORTRQ		8
#define RS_ST_MDBGS_CONN		9
#define RS_ST_WT_PORTACK		10
#define RS_ST_ACCEPT_MDBG	11
#define RS_ST_CONNECTED		12

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

unsigned char chk_sock(int*, struct sockaddr_in*, struct sockaddr_in*);


// ***  MACROS  ***

#define ERR(msg_) \
{\
  fflush(stdout);\
  perror(msg_);\
}

#define DIE(errno_, errmsg_) \
{\
  ERR(errmsg_)\
  exit(errno_);\
}

#if DEBUG > 0

# define LOGPRINTF(...) \
{\
  printf(__VA_ARGS__);\
  fflush(stdout);\
}

# define DEBUGPRINTF(...) \
{\
  printf(__VA_ARGS__);\
  fflush(stdout);\
}

#else

# define LOGPRINTF(...) \
{\
  printf(__VA_ARGS__);\
}

# define DEBUGPRINTF(...) ;

#endif	// #if DEBUG == 1


#endif	// #ifndef REVSOCKET_H
