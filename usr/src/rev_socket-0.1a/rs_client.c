
// Reverse socket client

#include "rev_socket.h"

unsigned char chk_sock(int *sock, struct sockaddr_in *my_addr, struct sockaddr_in *upl_addr)
{
	if (*sock <= 0)
	{
		if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
		{
			ERR("socket")
			return 0;
		}
		if (bind(*sock, (struct sockaddr*)my_addr, sizeof(struct sockaddr)) < 0)
		{
			ERR("bind")
			*sock = 0;
			return 0;
		}
		if (connect(*sock, (struct sockaddr*)upl_addr, sizeof(struct sockaddr)) < 0)
		{
			ERR("connect")
			*sock = 0;
			return 0;
		}
		fcntl(*sock, F_SETFL, fcntl(*sock, F_GETFL) | O_NONBLOCK);
#define IPADDR ((unsigned char*)&(upl_addr->sin_addr.s_addr))
		DEBUGPRINTF("  <%02d> Socket connected to host %hhu.%hhu.%hhu.%hhu\n", *sock, IPADDR[0], IPADDR[1], IPADDR[2], IPADDR[3])
		return 1;
	}	// if (*sock <= 0)
	return 1;	// socket already open
}	// chk_sock()

int main(int argc, char *argv[])
{

if ( argc > 1 && !strcmp( argv[1], "-V" ))
{
	LOGPRINTF( "%s\n", PRGVER )
	exit(EXIT_SUCCESS);
}

if ( argc != 4 || atoi(argv[1]) == 0 || atoi(argv[1]) > 65535 || atoi(argv[3]) == 0 || atoi(argv[3]) > 65535 )
{
	fprintf(stderr, "Usage: %s <moviDebugServer port> <rs_server address> <rs_server port>\n", argv[0]);
	exit(EXIT_FAILURE);
}

unsigned short mdbgs_port = atoi(argv[1]), srv_port = atoi(argv[3]);
int i, s, srv_sock = 0, mdbgs_sock = 0, srv_len = 0, mdbgs_len = 0;
fd_set fds, wfds;
struct timeval tv;
struct sockaddr_in srv_addr, mdbgs_addr, my_addr;
struct hostent *he;
unsigned char state = RS_ST_CONNECT, ping_ctr = RS_RETRY_INTERVAL;
char recv_buf[BUF_SIZE];
char send_buf[BUF_SIZE];

	LOGPRINTF("Reverse socket client v%s\n", PRGVER)
	LOGPRINTF("  MDBGS port: %hu\n", mdbgs_port)
	LOGPRINTF("  Server address: %s\n", argv[2])
	LOGPRINTF("  Server port: %hu\n", srv_port)

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = 0;
	my_addr.sin_addr.s_addr= htonl(INADDR_ANY);
	bzero(&(my_addr.sin_zero), 8);

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(srv_port);
	he = gethostbyname(argv[2]);
	if (!he)
	{
		fprintf(stderr, "Error: unknown host address %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}
	memcpy((char*) &(srv_addr.sin_addr.s_addr), *he->h_addr_list, sizeof(struct in_addr));
	bzero(&(srv_addr.sin_zero), 8);

	mdbgs_addr.sin_family = AF_INET;
	mdbgs_addr.sin_port = htons(mdbgs_port);
	mdbgs_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	bzero(&(mdbgs_addr.sin_zero), 8);

// *** STARTING SERVICE ***

	tv.tv_usec = TV_USEC;
	tv.tv_sec = TV_SEC_0;
	while(1)
	{
		// Preparing data for select()
		FD_ZERO(&fds);
		FD_ZERO(&wfds);
		FD_SET(srv_sock, &fds);
		if (mdbgs_len) FD_SET(srv_sock, &wfds);
		s = srv_sock;
		if (mdbgs_sock > 0)
		{
			FD_SET(mdbgs_sock, &fds);
			if (srv_len) FD_SET(mdbgs_sock, &wfds);
			if (mdbgs_sock > s) s = mdbgs_sock;
		}

		if (select(s+1, &fds, &wfds, NULL, &tv) < 0) DIE(EXIT_FAILURE, "select")
		else {

			tv.tv_usec = TV_USEC;
			tv.tv_sec = TV_SEC_TICK;
			switch (state)
			{

			case RS_ST_CONNECT:
				if (++ping_ctr > RS_RETRY_INTERVAL)
				{
					ping_ctr = 0;
					if (chk_sock(&srv_sock, &my_addr, &srv_addr))
					{
						LOGPRINTF("Socket %02d connected to %s:%hu\n", srv_sock, argv[2], srv_port)
						state = RS_ST_WT_LOGIN;
					} else break;
				} else break;

			case RS_ST_WT_LOGIN:
				if (FD_ISSET(srv_sock, &fds))
				{
					FD_CLR(srv_sock, &fds);
					if ((i = recv(srv_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						LOGPRINTF("  <%02d> Server disconnected\n", srv_sock)
						close(srv_sock);
						srv_sock = 0;
						tv.tv_sec = TV_SEC_0;
						ping_ctr = RS_RETRY_INTERVAL;
						state = RS_ST_CONNECT;
						break;
					}
					if (i==2)
					{
						DEBUGPRINTF("  <%02d> Received login 0x%04hX\n", srv_sock, ntohs(*((unsigned short*)recv_buf)))
						*((unsigned short*)recv_buf) ^= htons(RS_MAGIC);
						send(srv_sock, recv_buf, 2, 0);
					}
					state = RS_ST_WT_PING;
					ping_ctr = 0;
				} else break;

			case RS_ST_WT_PING:
				if (FD_ISSET(srv_sock, &fds))
				{
					FD_CLR(srv_sock, &fds);
					if ((i = recv(srv_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						LOGPRINTF("  <%02d> Disconnected\n", srv_sock)
						close(srv_sock);
						srv_sock = 0;
						tv.tv_sec = TV_SEC_0;
						ping_ctr = RS_RETRY_INTERVAL;
						state = RS_ST_CONNECT;
						break;
					}
					if (i==2)
					{
						if (*((unsigned short*)recv_buf) == 0)
						{
							*((unsigned short*)send_buf) = 0;
							send(srv_sock, send_buf, 2, 0);
							ping_ctr = 0;
							break;
						} else {
							state = RS_ST_MDBGS_CONN;
						}
					} else break;

				} else {
					if (++ping_ctr > RS_SOCKET_TO)
					{
						LOGPRINTF("  <%02d> PING timeout\n", srv_sock)
						shutdown(srv_sock, SHUT_RDWR);
						close(srv_sock);
						srv_sock = 0;
						tv.tv_sec = TV_SEC_0;
						ping_ctr = RS_RETRY_INTERVAL;
						state = RS_ST_CONNECT;
					}
					break;
				}

			case RS_ST_MDBGS_CONN:
				if (FD_ISSET(srv_sock, &fds))
				{
					FD_CLR(srv_sock, &fds);
					if ((i = recv(srv_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						LOGPRINTF("  <%02d> Server disconnected while trying to connect\n", srv_sock)
						close(srv_sock);
						srv_sock = 0;
						tv.tv_sec = TV_SEC_0;
						ping_ctr = RS_RETRY_INTERVAL;
						state = RS_ST_CONNECT;
						break;
					} else {
						DEBUGPRINTF("  <%02d> %d bytes of data on srv_sock\n", srv_sock, i)
					}
				}
				if (chk_sock(&mdbgs_sock, &my_addr, &mdbgs_addr))
				{
					LOGPRINTF("  <%02d-%02d> Connected to MDBG Server\n", srv_sock, mdbgs_sock)
					state = RS_ST_WT_LOGIN;
					*((unsigned short*)send_buf) = mdbgs_addr.sin_port;
					send(srv_sock, send_buf, 2, 0);
					state = RS_ST_CONNECTED;
				} else {
					mdbgs_sock = 0;
					break;
				}

			case RS_ST_CONNECTED:
				if (!srv_len && FD_ISSET(srv_sock, &fds))
				{
					FD_CLR(srv_sock, &fds);
					if ((srv_len = recv(srv_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						shutdown(mdbgs_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> Server disconnected\n", srv_sock, mdbgs_sock)
						recv(mdbgs_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					}
				}
				if (!mdbgs_len && FD_ISSET(mdbgs_sock, &fds))
				{
					FD_CLR(mdbgs_sock, &fds);
					if ((mdbgs_len = recv(mdbgs_sock, send_buf, BUF_SIZE, 0)) <= 0)
					{
						shutdown(srv_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> MDBG disconnected\n", srv_sock, mdbgs_sock)
						recv(srv_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					}
				}
				if (srv_len && FD_ISSET(mdbgs_sock, &wfds))
				{
					FD_CLR(mdbgs_sock, &wfds);
					if ((i = send(mdbgs_sock, recv_buf, srv_len, 0)) < 0)
					{
						shutdown(srv_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> MDBG send failed (%d)\n", srv_sock, mdbgs_sock, i)
						recv(srv_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					} else if (i) {
						if (i != srv_len) LOGPRINTF("  <%02d-%02d> Srv only sent %d out of %d\n", srv_sock, mdbgs_sock, i, srv_len)
						srv_len = 0;
					}
				}
				if (mdbgs_len && FD_ISSET(srv_sock, &wfds))
				{
					FD_CLR(srv_sock, &wfds);
					if ((i = send(srv_sock, send_buf, mdbgs_len, 0)) < 0)
					{
						shutdown(mdbgs_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> Server send failed (%d)\n", srv_sock, mdbgs_sock, i)
						recv(mdbgs_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					} else if (i) {
						if (i != mdbgs_len) LOGPRINTF("  <%02d-%02d> MDBG only sent %d out of %d\n", srv_sock, mdbgs_sock, i, mdbgs_len)
						mdbgs_len = 0;
					}
				}
				break;

				CONN_ERR:
				close(srv_sock);
				srv_sock = 0;
				close(mdbgs_sock);
				mdbgs_sock = 0;
				tv.tv_sec = TV_SEC_0;
				ping_ctr = RS_RETRY_INTERVAL;
				srv_len = mdbgs_len = 0;
				state = RS_ST_CONNECT;
			break;

			}	// switch (state)

		}	// select

	}	// while(1)

}	// main()
