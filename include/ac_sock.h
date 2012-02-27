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


#ifndef __APP_PKT_T_
#define __APP_PKT_T_

#include <unistd.h>
#define __USE_GNU
#include <sys/socket.h>
#include <linux/un.h>

enum ac_cmd {
	AC_CHECK,
	AC_REGISTER,
	AC_UNREGISTER,
};

#define AC_SOCK_NAME "/tmp/ac-socket"
#define AC_SOCK_MAXBUFF 65535

typedef struct _ac_pkt_t {
	int cmd;
	int len;
	unsigned char data[1];
} ac_pkt_t;

int _create_server_sock();
int _create_client_sock();
int _app_send_raw(int cmd, unsigned char *data, int datalen);
ac_pkt_t *_app_recv_raw(int fd, int *clifd, struct ucred *cr);
int _send_result_to_server(int fd, int res);


#endif

