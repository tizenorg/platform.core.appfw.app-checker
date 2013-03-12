/*
 *  app-checker
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>, Jaeho Lee <jaeho81.lee@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>


#include "ac_sock.h"
#include "internal.h"

static int __connect_client_sock(int sockfd, const struct sockaddr *saptr, socklen_t salen,
		   int nsec);

static inline void __set_sock_option(int fd, int cli)
{
	int size;
	struct timeval tv = { 5, 200 * 1000 };	/*  5.2 sec */

	size = AC_SOCK_MAXBUFF;
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	if (cli)
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int _create_server_sock()
{
	struct sockaddr_un saddr;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	/*  support above version 2.6.27*/
	if (fd < 0) {
		if(errno == EINVAL) {
			fd = socket(AF_UNIX, SOCK_STREAM, 0);
			if(fd < 0) {
				_E("second chance - socket create error");
				return -1;
			}
		} else {
			_E("socket error");
			return -1;
		}
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sun_family = AF_UNIX;
	snprintf(saddr.sun_path, UNIX_PATH_MAX, "%s",AC_SOCK_NAME);
	unlink(saddr.sun_path);
	
	if (bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		_E("bind error");
		close(fd);
		return -1;
	}

	if (chmod(saddr.sun_path, (S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		/* Flawfinder: ignore*/
		_E("failed to change the socket permission");
		close(fd);
		return -1;
	}

	__set_sock_option(fd, 0);

	if (listen(fd, 10) == -1) {
		_E("listen error");
		close(fd);
		return -1;
	}	

	return fd;
}

int _create_client_sock()
{
	int fd = -1;
	struct sockaddr_un saddr = { 0, };
	int retry = 1;
	int ret = -1;

	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);	
	/*  support above version 2.6.27*/
	if (fd < 0) {
		if (errno == EINVAL) {
			fd = socket(AF_UNIX, SOCK_STREAM, 0);
			if (fd < 0) {
				_E("second chance - socket create error");
				return -1;
			}
		} else {
			_E("socket error");
			return -1;
		}
	}

	saddr.sun_family = AF_UNIX;
	snprintf(saddr.sun_path, UNIX_PATH_MAX, "%s", AC_SOCK_NAME);
 retry_con:
	ret = __connect_client_sock(fd, (struct sockaddr *)&saddr, sizeof(saddr),
			100 * 1000);
	if (ret < -1) {
		_E("maybe peer not launched or peer daed\n");
		if (retry > 0) {
			usleep(100 * 1000);
			retry--;
			goto retry_con;
		}
	}
	if (ret < 0) {
		close(fd);
		return -1;
	}

	__set_sock_option(fd, 1);

	return fd;
}

static int __connect_client_sock(int fd, const struct sockaddr *saptr, socklen_t salen,
		   int nsec)
{
	int flags;
	int ret;
	int error;
	socklen_t len;
	fd_set readfds;
	fd_set writefds;
	struct timeval timeout;

	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	error = 0;
	if ((ret = connect(fd, (struct sockaddr *)saptr, salen)) < 0) {
		if (errno != EAGAIN && errno != EINPROGRESS) {
			fcntl(fd, F_SETFL, flags);	
			return (-2);
		}
	}

	/* Do whatever we want while the connect is taking place. */
	if (ret == 0)
		goto done;	/* connect completed immediately */

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	writefds = readfds;
	timeout.tv_sec = 0;
	timeout.tv_usec = nsec;

	if ((ret = select(fd + 1, &readfds, &writefds, NULL, 
			nsec ? &timeout : NULL)) == 0) {
		close(fd);	/* timeout */
		errno = ETIMEDOUT;
		return (-1);
	}

	if (FD_ISSET(fd, &readfds) || FD_ISSET(fd, &writefds)) {
		len = sizeof(error);
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
			return (-1);	/* Solaris pending error */
	} else
		return (-1);	/* select error: sockfd not set*/

 done:
	fcntl(fd, F_SETFL, flags);	
	if (error) {
		close(fd);	
		errno = error;
		return (-1);
	}
	return (0);
}

/**
 * @brief	Send data (in raw) to the process with 'pid' via socket
 */
int _app_send_raw(int cmd, unsigned char *data, int datalen)
{
	int fd;
	int len;
	int res = 0;
	ac_pkt_t *pkt = NULL;

	if (data == NULL || datalen > AC_SOCK_MAXBUFF - 8) {
		_E("keybundle error\n");
		return -EINVAL;
	}

	fd = _create_client_sock();
	if (fd < 0)
		return -ECOMM;

	pkt = (ac_pkt_t *) malloc(sizeof(char) * AC_SOCK_MAXBUFF);
	if (NULL == pkt) {
		_E("Malloc Failed!");
		return -ENOMEM;
	}
	memset(pkt, 0, AC_SOCK_MAXBUFF);

	pkt->cmd = cmd;
	pkt->len = datalen;
	memcpy(pkt->data, data, datalen);

	if ((len = send(fd, pkt, datalen + 8, 0)) != datalen + 8) {
		_E("sendto() failed - %d %d", len, datalen + 8);
		if (errno == EPIPE) {
			_E("fd:%d\n", fd);
		}
		close(fd);
		if (pkt) {
			free(pkt);
			pkt = NULL;
		}
		return -ECOMM;
	}
	if (pkt) {
		free(pkt);
		pkt = NULL;
	}

	len = recv(fd, &res, sizeof(int), 0);
	if (len == -1) {
		if (errno == EAGAIN) {
			_E("recv timeout \n");
			res = -EAGAIN;
		} else {
			_E("recv error\n");
			res = -ECOMM;
		}
	} else
		_D("recv result  = %d (%d)", res, len);
	close(fd);

	return res;
}

ac_pkt_t *_app_recv_raw(int fd, int *clifd, struct ucred *cr)
{
	int len;
	struct sockaddr_un aul_addr = { 0, };
	int sun_size;
	ac_pkt_t *pkt = NULL;
	int cl = sizeof(struct ucred);

	sun_size = sizeof(struct sockaddr_un);

	if ((*clifd = accept(fd, (struct sockaddr *)&aul_addr,
			     (socklen_t *) &sun_size)) == -1) {
		if (errno != EINTR)
			_E("accept error");
		return NULL;
	}

	if (getsockopt(*clifd, SOL_SOCKET, SO_PEERCRED, cr,
		       (socklen_t *) &cl) < 0) {
		_E("peer information error");
		close(*clifd);
		return NULL;
	}

	pkt = (ac_pkt_t *) malloc(sizeof(char) * AC_SOCK_MAXBUFF);
	if(pkt == NULL) {
		close(*clifd);
		return NULL;
	}
	memset(pkt, 0, AC_SOCK_MAXBUFF);

	__set_sock_option(*clifd, 1);

 retry_recv:
	/* receive single packet from socket */
	len = recv(*clifd, pkt, AC_SOCK_MAXBUFF, 0);
	if (len < 0)
		if (errno == EINTR)
			goto retry_recv;

	if ((len < 8) || (len != (pkt->len + 8))) {
		_E("recv error %d %d", len, pkt->len);
		free(pkt);
		close(*clifd);
		return NULL;
	}

	return pkt;
}

int _send_result_to_server(int fd, int res)
{
	if (send(fd, &res, sizeof(int), MSG_NOSIGNAL) < 0) {
		if (errno == EPIPE)
			_E("send failed due to EPIPE.\n");
		_E("send fail to client");
	}
	close(fd);
	return 0;
}
