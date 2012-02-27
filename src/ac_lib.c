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


#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <errno.h>

#include <app-checker.h>
#include "ac_sock.h"
#include "internal.h"

#ifndef SLPAPI
#define SLPAPI __attribute__ ((visibility("default")))
#endif

static int app_send_cmd(const char *pkg_name, const char *pkg_type, int pid, int cmd)
{
	int ret = -1;
	char *data;
	struct ac_data ad;

	strncpy(ad.pkg_name, pkg_name, MAX_PACKAGE_STR_SIZE);
	strncpy(ad.pkg_type, pkg_type, MAX_PACKAGE_TYPE_SIZE);
	ad.pid = pid;

	data = g_base64_encode(&ad, sizeof(ad));
	
	if ((ret = _app_send_raw(cmd, data, strnlen(data, AC_SOCK_MAXBUFF - 8))) < 0) {
		switch (ret) {
		case -EINVAL:
			ret = AC_R_EINVAL;
			break;
		case -ECOMM:
			ret = AC_R_ECOMM;
			break;
		case -EAGAIN:
			ret = AC_R_ETIMEOUT;
			break;
		case AC_R_ENOPULUGINS:
			ret = AC_R_ENOPULUGINS;
			break;
		default:
			ret = AC_R_ERROR;
		}
	}

	g_free(data);
	return ret;
}

SLPAPI int ac_check_launch_privilege(const char *pkg_name, const char *pkg_type, int pid)
{
	int ret = -1;

	if(pkg_name == NULL || pkg_type == NULL)
		return AC_R_EINVAL;

	ret = app_send_cmd(pkg_name, pkg_type, pid, AC_CHECK);

	return ret;
}

SLPAPI int ac_register_launch_privilege(const char *pkg_name, const char *pkg_type)
{
	int ret = -1;

	if(pkg_name == NULL || pkg_type == NULL)
		return AC_R_EINVAL;

	ret = app_send_cmd(pkg_name, pkg_type, -1, AC_REGISTER);

	return ret;
}

SLPAPI int ac_unregister_launch_privilege(const char *pkg_name, const char *pkg_type)
{
	int ret = -1;

	if(pkg_name == NULL || pkg_type == NULL)
		return AC_R_EINVAL;

	ret = app_send_cmd(pkg_name, pkg_type, -1, AC_UNREGISTER);

	return ret;	
}

