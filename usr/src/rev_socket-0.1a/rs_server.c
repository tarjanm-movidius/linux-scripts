
// Reverse socket server

#include "rev_socket.h"


int main(int argc, char *argv[])
{

if (argc > 1 && !strcmp( argv[1], "-V"))
{
	LOGPRINTF( "%s\n", PRGVER )
	exit(EXIT_SUCCESS);
}

if (argc != 3 || atoi(argv[1]) == 0 || atoi(argv[1]) > 65535 || atoi(argv[2]) == 0 || atoi(argv[2]) > 65535)
{
	fprintf(stderr, "Usage: %s <moviDebug port> <rs_client port>\n", argv[0]);
	exit(EXIT_FAILURE);
}

unsigned short mdbg_port = atoi(argv[1]), clnt_port = atoi(argv[2]);
int i, s, clnt_lsock, clnt_sock = 0, mdbg_lsock, mdbg_sock = 0, clnt_len = 0, mdbg_len = 0;
fd_set fds, wfds;
struct timeval tv;
struct sockaddr_in clnt_addr, mdbg_addr;
unsigned char state = RS_ST_CLNT_WT, ping_ctr = 0;
char recv_buf[BUF_SIZE];
char send_buf[BUF_SIZE];

	LOGPRINTF("Reverse socket server v%s\n", PRGVER)
	LOGPRINTF("  MDBG port: %hu\n", mdbg_port)
	LOGPRINTF("  Client port: %hu\n", clnt_port)

	mdbg_addr.sin_family = AF_INET;
	mdbg_addr.sin_port = htons(mdbg_port);
	mdbg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(mdbg_addr.sin_zero), 8);

	clnt_addr.sin_family = AF_INET;
	clnt_addr.sin_port = htons(clnt_port);
	clnt_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(clnt_addr.sin_zero), 8);

// *** CREATING LISTEN SOCKETS ***
	if ((mdbg_lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) DIE(EXIT_FAILURE, "socket(mdbg)")
	setsockopt(mdbg_lsock, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s));
	fcntl(mdbg_lsock, F_SETFL, fcntl(mdbg_lsock, F_GETFL) | O_NONBLOCK);
	DEBUGPRINTF("  mdbg_lsocket %02d created, ", mdbg_lsock)
	if (bind(mdbg_lsock, (struct sockaddr*) &mdbg_addr, sizeof(struct sockaddr)) < 0) DIE(EXIT_FAILURE, "bind(mdbg)")
	listen(mdbg_lsock, 1);
	DEBUGPRINTF("listening on port %d\n", mdbg_port)

	if ((clnt_lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) DIE(EXIT_FAILURE, "socket(clnt)")
	setsockopt(clnt_lsock, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s));
	fcntl(clnt_lsock, F_SETFL, fcntl(clnt_lsock, F_GETFL) | O_NONBLOCK);
	DEBUGPRINTF("  clnt_lsocket %02d created, ", clnt_lsock)
	if (bind(clnt_lsock, (struct sockaddr*) &clnt_addr, sizeof(struct sockaddr)) < 0) DIE(EXIT_FAILURE, "bind(clnt)")
	listen(clnt_lsock, 1);
	DEBUGPRINTF("listening on port %d\n", clnt_port)

// *** STARTING SERVICE ***

	tv.tv_usec = TV_USEC;
	while(1)
	{
		// Preparing data for select()
		FD_ZERO(&fds);
		FD_ZERO(&wfds);
		FD_SET(clnt_lsock, &fds); s = clnt_lsock;
		FD_SET(mdbg_lsock, &fds); if (mdbg_lsock > s) s = mdbg_lsock;
		if (clnt_sock > 0)
		{
			FD_SET(clnt_sock, &fds);
			if (mdbg_len) FD_SET(clnt_sock, &wfds);	// xxxx_len is data received
			if (clnt_sock > s) s = clnt_sock;
		}
		if (mdbg_sock > 0)
		{
			FD_SET(mdbg_sock, &fds);
			if (clnt_len) FD_SET(mdbg_sock, &wfds);
			if (mdbg_sock > s) s = mdbg_sock;
		}

		if (select(s+1, &fds, &wfds, NULL, &tv) < 0) DIE(EXIT_FAILURE, "select")
		else {

			tv.tv_sec = TV_SEC_TICK;
			switch (state)
			{

			case RS_ST_CLNT_WT:
				if ( FD_ISSET(clnt_lsock, &fds) )	// new connection acccepted
				{
					FD_CLR(clnt_lsock, &fds);
					if (clnt_sock > 0)
					{
						DEBUGPRINTF("  <%02d> Closing left-open socket\n", clnt_sock)
						shutdown(clnt_sock, SHUT_RDWR);
						close(clnt_sock);
					}
					if ((clnt_sock = accept( clnt_lsock, NULL, NULL )) < 0 )
					{
						ERR("accept(clnt)")
						break;
					}
					fcntl(clnt_sock, F_SETFL, fcntl(clnt_sock, F_GETFL) | O_NONBLOCK);
					DEBUGPRINTF("New connection accepted on socket: %02d\n", clnt_sock)
					state = RS_ST_SND_LOGIN;
				} else break;

			case RS_ST_SND_LOGIN:
				gettimeofday(&tv, NULL);
				*((unsigned short*)send_buf) = tv.tv_usec & 0xFFFF;
				tv.tv_usec = TV_USEC;
				send(clnt_sock, send_buf, 2, 0);
				DEBUGPRINTF("  <%02d> Sent login 0x%04hX\n", clnt_sock, ntohs(*((unsigned short*)send_buf)))
				state = RS_ST_WT_LOGIN;
			break;

			case RS_ST_WT_LOGIN:
				if (FD_ISSET(clnt_sock, &fds))
				{
					FD_CLR(clnt_sock, &fds);
					if ((i = recv(clnt_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						LOGPRINTF("  <%02d> Disconnected\n", clnt_sock)
						close(clnt_sock);
						clnt_sock = 0;
						state = RS_ST_CLNT_WT;
						break;
					}
					if (i!=2 || *((unsigned short*)recv_buf)!=(*((unsigned short*)send_buf) ^ htons(RS_MAGIC)))
					{
						LOGPRINTF("  <%02d> Login failed\n", clnt_sock)
						shutdown(clnt_sock, SHUT_RDWR);
						close(clnt_sock);
						clnt_sock = 0;
						state = RS_ST_CLNT_WT;
						break;
					}
#if DEBUG > 0
					LOGPRINTF("  <%02d> Logged in\n", clnt_sock)
#else
					LOGPRINTF("New connection accepted on socket: %02d\n", clnt_sock)
#endif
					state = RS_ST_PING;
				} else break;

			case RS_ST_PING:
				*((unsigned short*)send_buf) = 0;
				send(clnt_sock, send_buf, 2, 0);
				state = RS_ST_WT_PONG;
			break;

			case RS_ST_WT_PONG:
				if (FD_ISSET(clnt_sock, &fds))
				{
					FD_CLR(clnt_sock, &fds);
					if ((i = recv(clnt_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						LOGPRINTF("  <%02d> Disconnected\n", clnt_sock)
						close(clnt_sock);
						clnt_sock = 0;
						state = RS_ST_CLNT_WT;
						break;
					}
					if (i!=2 || *((unsigned short*)recv_buf)!=0)
					{
						LOGPRINTF("  <%02d> Bad PING respone\n", clnt_sock)
						shutdown(clnt_sock, SHUT_RDWR);
						close(clnt_sock);
						clnt_sock = 0;
						state = RS_ST_CLNT_WT;
						break;
					}
					ping_ctr = 0;
					state = RS_ST_WT_MDBG;
				} else {
					if (++ping_ctr > RS_SOCKET_TO)
					{
						LOGPRINTF("  <%02d> PING timeout\n", clnt_sock)
						shutdown(clnt_sock, SHUT_RDWR);
						close(clnt_sock);
						clnt_sock = 0;
						state = RS_ST_CLNT_WT;
					}
					break;
				}

			case RS_ST_WT_MDBG:
				if (FD_ISSET(mdbg_lsock, &fds))
				{
					FD_CLR(mdbg_lsock, &fds);
					DEBUGPRINTF("  <%02d> MDBG connection request received\n", clnt_sock)
					if (mdbg_sock > 0)
					{
						shutdown(mdbg_sock, SHUT_RDWR);
						close(mdbg_sock);
					}
					// We'll open the socket on the client side before accepting connection
					state = RS_ST_SND_PORTRQ;

				}
				if (FD_ISSET(clnt_sock, &fds)) {

					FD_CLR(clnt_sock, &fds);
					if ((i = recv(clnt_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						LOGPRINTF("  <%02d> Client disconnected while waiting for MDBG\n", clnt_sock)
						close(clnt_sock);
						clnt_sock = 0;
						state = RS_ST_CLNT_WT;
						break;
					} else {
						LOGPRINTF("  <%02d> %d bytes data on client socket while waiting for MDBG\n", clnt_sock, i)
						i = 0;
					}
				}
				if ((state == RS_ST_WT_MDBG) && (++ping_ctr > RS_PING_INTERVAL))
				{
					*((unsigned short*)send_buf) = 0;
					send(clnt_sock, send_buf, 2, 0);
					state = RS_ST_WT_PONG;
				}
			break;

			case RS_ST_SND_PORTRQ:
				*((unsigned short*)send_buf) = htons(1);
				send(clnt_sock, send_buf, 2, 0);
				state = RS_ST_WT_PORTACK;
			break;

			case RS_ST_WT_PORTACK:
				if (FD_ISSET(clnt_sock, &fds))
				{
					FD_CLR(clnt_sock, &fds);
					if ((i = recv(clnt_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						LOGPRINTF("  <%02d> Disconnected\n", clnt_sock)
						close(clnt_sock);
						clnt_sock = 0;
/*
						shutdown(mdbg_sock, SHUT_RDWR);
						close(mdbg_sock);
						mdbg_sock = 0;
*/
						state = RS_ST_CLNT_WT;
						break;
					}
					if (i!=2 || *((unsigned short*)recv_buf)==0)
					{
						LOGPRINTF("  <%02d> Opening port failed on client side\n", clnt_sock)
/*
						shutdown(clnt_sock, SHUT_RDWR);
						close(clnt_sock);
						clnt_sock = 0;
*/
						state = RS_ST_WT_MDBG;
						break;
					}
					state = RS_ST_ACCEPT_MDBG;
				} else break;

			case RS_ST_ACCEPT_MDBG:
				if ((mdbg_sock = accept( mdbg_lsock, NULL, NULL )) < 0 )
				{
					ERR("accept(mdbg)")
					state = RS_ST_WT_MDBG;
					break;
				}
				fcntl(mdbg_sock, F_SETFL, fcntl(mdbg_sock, F_GETFL) | O_NONBLOCK);
				LOGPRINTF("  <%02d-%02d> MDBG socket connected to remote port %hu\n", clnt_sock, mdbg_sock, ntohs(*((unsigned short*)recv_buf)))
				state = RS_ST_CONNECTED;
			break;

			case RS_ST_CONNECTED:
				if (!clnt_len && FD_ISSET(clnt_sock, &fds))
				{
					FD_CLR(clnt_sock, &fds);
					if ((clnt_len = recv(clnt_sock, recv_buf, BUF_SIZE, 0)) <= 0)
					{
						shutdown(mdbg_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> Client disconnected\n", clnt_sock, mdbg_sock)
						recv(mdbg_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					}
				}
				if (!mdbg_len && FD_ISSET(mdbg_sock, &fds))
				{
					FD_CLR(mdbg_sock, &fds);
					if ((mdbg_len = recv(mdbg_sock, send_buf, BUF_SIZE, 0)) <= 0)
					{
						shutdown(clnt_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> MDBG disconnected\n", clnt_sock, mdbg_sock)
						recv(clnt_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					}
				}
				if (clnt_len && FD_ISSET(mdbg_sock, &wfds))
				{
					FD_CLR(mdbg_sock, &wfds);
					if ((i = send(mdbg_sock, recv_buf, clnt_len, 0)) < 0)
					{
						shutdown(clnt_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> MDBG send failed (%d)\n", clnt_sock, mdbg_sock, i)
						recv(clnt_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					} else if (i) {
						if (i != clnt_len) LOGPRINTF("  <%02d-%02d> Clnt only sent %d out of %d\n", clnt_sock, mdbg_sock, i, clnt_len)
						clnt_len = 0;
					}
				}
				if (mdbg_len && FD_ISSET(clnt_sock, &wfds))
				{
					FD_CLR(clnt_sock, &wfds);
					if ((i = send(clnt_sock, send_buf, mdbg_len, 0)) < 0)
					{
						shutdown(mdbg_sock, SHUT_RDWR);
						LOGPRINTF("  <%02d-%02d> Client send failed (%d)\n", clnt_sock, mdbg_sock, i)
						recv(mdbg_sock, recv_buf, BUF_SIZE, 0);
						goto CONN_ERR;
					} else if (i) {
						if (i != mdbg_len) LOGPRINTF("  <%02d-%02d> MDBG only sent %d out of %d\n", clnt_sock, mdbg_sock, i, mdbg_len)
						mdbg_len = 0;
					}
				}
				break;

				CONN_ERR:
				DEBUGPRINTF("Closing sockets %02d-%02d\n", clnt_sock, mdbg_sock)
				close(mdbg_sock);
				close(clnt_sock);
				mdbg_sock = 0;
				clnt_sock = 0;
				clnt_len = mdbg_len = 0;
				state = RS_ST_CLNT_WT;
			break;

			}	// switch (state)

		}	// select

	}	// while(1)

}	// main()
