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
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "app-checker.h"
#include "ac_sock.h"
#include "internal.h"

#define PLUGINS_PREFIX LIBPREFIX "/ac-plugins"
#define MAX_LOCAL_BUFSZ 512

#ifndef SLPAPI
#define SLPAPI __attribute__ ((visibility("default")))
#endif

GSList *pkg_type_list = NULL;

typedef struct _ac_type_list_t {
	char *pkg_type;
	GSList *so_list;
} ac_type_list_t;

typedef struct _ac_so_list_t {
	char *so_name;
	int (*ac_check)(const char*);
	int (*ac_register)(const char*);
	int (*ac_unregister)(const char*);
} ac_so_list_t;

static int __send_to_sigkill(int pid)
{
	int pgid;

	pgid = getpgid(pid);
	if (pgid <= 1)
		return -1;

	if (killpg(pgid, SIGKILL) < 0)
		return -1;

	return 0;
}

static int __check_launch_privilege(const char *pkg_name,
		const char *pkg_type, int pid)
{
	GSList *iter = NULL;
	GSList *iter2 = NULL;
	ac_type_list_t *type_t;
	ac_so_list_t *so_t;

	for (iter = pkg_type_list; iter != NULL; iter = g_slist_next(iter)) {
		type_t = iter->data;
		if (strncmp(type_t->pkg_type, pkg_type, MAX_PACKAGE_TYPE_SIZE) == 0) {
			for (iter2 = type_t->so_list; iter2 != NULL; iter2 = g_slist_next(iter2)) {
				so_t = iter2->data;
				_D("type : %s / so name : %s / func : %x", type_t->pkg_type, so_t->so_name, so_t->ac_check);
				if (so_t->ac_check && so_t->ac_check(pkg_name) < 0) {
					if (pid > 0)
						__send_to_sigkill(pid);
					return AC_R_ERROR;
				}
			}
			return AC_R_OK;
		}
	}

	return AC_R_ENOPULUGINS;
}

static int __register_launch_privilege(const char *pkg_name, const char *pkg_type)
{
	GSList *iter = NULL;
	GSList *iter2 = NULL;
	ac_type_list_t *type_t;
	ac_so_list_t *so_t;
	int ret = AC_R_OK;

	for (iter = pkg_type_list; iter != NULL; iter = g_slist_next(iter)) {
		type_t = iter->data;
		if (strncmp(type_t->pkg_type, pkg_type, MAX_PACKAGE_TYPE_SIZE) == 0) {
			for (iter2 = type_t->so_list; iter2 != NULL; iter = g_slist_next(iter2)) {
				so_t = iter2->data;
				if (so_t->ac_register && so_t->ac_register(pkg_name) < 0)
					ret = AC_R_ERROR;
			}
			return ret;
		}
	}

	return AC_R_ENOPULUGINS;

}

static int __unregister_launch_privilege(const char *pkg_name, const char *pkg_type)
{
	GSList *iter = NULL;
	GSList *iter2 = NULL;
	ac_type_list_t *type_t;
	ac_so_list_t *so_t;
	int ret = AC_R_OK;

	for (iter = pkg_type_list; iter != NULL; iter = g_slist_next(iter)) {
		type_t = iter->data;
		if (strncmp(type_t->pkg_type, pkg_type, MAX_PACKAGE_TYPE_SIZE) == 0) {
			for (iter2 = type_t->so_list; iter2 != NULL; iter = g_slist_next(iter2)) {
				so_t = iter2->data;
				if (so_t->ac_unregister && so_t->ac_unregister(pkg_name) < 0)
					ret = AC_R_ERROR;
			}
			return ret;
		}
	}

	return AC_R_ENOPULUGINS;;

}

static gboolean __ac_handler(gpointer data)
{
	GPollFD *gpollfd = (GPollFD *) data;
	int fd = gpollfd->fd;
	ac_pkt_t *pkt;
	int clifd;
	struct ucred cr;
	struct ac_data *ad;
	int ret = -1;
	int size;

	if ((pkt = _app_recv_raw(fd, &clifd, &cr)) == NULL) {
		_E("recv error");
		return FALSE;
	}

	ad = (struct ac_data *)g_base64_decode((const gchar*)pkt->data, (gsize *)&size);
	if (ad == NULL) {
		ret = -1;
		goto ERROR;
	}

	_D("cmd : %d, pkgname : %s, pkgtype : %s", pkt->cmd, ad->pkg_name, ad->pkg_type);

	switch (pkt->cmd) {
	case AC_CHECK:
		_send_result_to_server(clifd, AC_R_OK);
		ret = __check_launch_privilege(ad->pkg_name, ad->pkg_type, ad->pid);
		g_free(ad);
		free(pkt);
		return TRUE;
		break;
	case AC_REGISTER:
		ret = __register_launch_privilege(ad->pkg_name, ad->pkg_type);
		break;
	case AC_UNREGISTER:
		ret = __unregister_launch_privilege(ad->pkg_name, ad->pkg_type);
		break;
	default:
		_E("no support packet");
	}
ERROR:
	_send_result_to_server(clifd, ret);
	if (ad)
		g_free(ad);
	if (pkt)
		free(pkt);
	return TRUE;
}

static gboolean __ac_glib_check(GSource *src)
{
	GSList *fd_list;
	GPollFD *tmp;

	fd_list = src->poll_fds;
	do {
		tmp = (GPollFD *) fd_list->data;
		if ((tmp->revents & (POLLIN | POLLPRI)))
			return TRUE;
		fd_list = fd_list->next;
	} while (fd_list);

	return FALSE;
}

static gboolean __ac_glib_dispatch(GSource *src, GSourceFunc callback,
				  gpointer data)
{
	callback(data);
	return TRUE;
}

static gboolean __ac_glib_prepare(GSource *src, gint *timeout)
{
	return FALSE;
}

static GSourceFuncs funcs = {
	.prepare = __ac_glib_prepare,
	.check = __ac_glib_check,
	.dispatch = __ac_glib_dispatch,
	.finalize = NULL
};

static void __pkt_type_list_free()
{
	GSList *iter = NULL;
	GSList *iter2 = NULL;
	ac_type_list_t *type_t;
	ac_so_list_t *so_t;

	for (iter = pkg_type_list; iter != NULL; iter = g_slist_next(iter)) {
		type_t = iter->data;
		if (type_t) {
			for (iter2 = type_t->so_list; iter2 != NULL; iter2 = g_slist_next(iter2)) {
				so_t = iter2->data;
				if (so_t) {
					if (so_t->so_name)
						free(so_t->so_name);
					free(so_t);
				}
			}
			g_slist_free(type_t->so_list);

			if (type_t->pkg_type)
				free(type_t->pkg_type);
			free(type_t);
		}
	}
	g_slist_free(pkg_type_list);
	return;
}

int __initialize()
{
	int fd;
	GPollFD *gpollfd;
	GSource *src;
	int ret;
	DIR *dp;
	struct dirent dentry;
	struct dirent *result = NULL;
	DIR *sub_dp = NULL;
	struct dirent sub_dentry;
	struct dirent *sub_result = NULL;
	char buf[MAX_LOCAL_BUFSZ];
	char buf2[MAX_LOCAL_BUFSZ];
	ac_type_list_t *type_t = NULL;
	void *handle = NULL;
	ac_so_list_t *so_t = NULL;

	_D("app checker server initialize");

	fd = _create_server_sock();
	if (fd == -1) {
		_E("_create_server_sock failed.");
		return AC_R_ERROR;
	}

	src = g_source_new(&funcs, sizeof(GSource));

	gpollfd = (GPollFD *) g_malloc(sizeof(GPollFD));
	if (!gpollfd) {
		g_source_unref(src);
		close(fd);
		return AC_R_ERROR;
	}
	gpollfd->events = POLLIN;
	gpollfd->fd = fd;

	g_source_add_poll(src, gpollfd);
	g_source_set_callback(src, (GSourceFunc) __ac_handler,
			      (gpointer) gpollfd, NULL);
	g_source_set_priority(src, G_PRIORITY_DEFAULT);

	ret = g_source_attach(src, NULL);
	if (ret == 0) {
		/* TODO: error handle */
		return AC_R_ERROR;
	}

	g_source_unref(src);

	dp = opendir(PLUGINS_PREFIX);
	if (dp == NULL)
		return AC_R_ERROR;

	while (readdir_r(dp, &dentry, &result) == 0 && result != NULL) {
		if (dentry.d_type != DT_DIR)
			continue;
		if (strcmp(dentry.d_name, ".") == 0
				|| strcmp(dentry.d_name, "..") == 0)
			continue;

		snprintf(buf, MAX_LOCAL_BUFSZ, "%s/%s",
				PLUGINS_PREFIX, dentry.d_name);
		_D("type : %s", dentry.d_name);

		type_t = malloc(sizeof(ac_type_list_t));
		if (type_t == NULL) {
			__pkt_type_list_free();
			closedir(dp);
			return AC_R_ERROR;
		}
		memset(type_t, 0, sizeof(ac_type_list_t));
		type_t->pkg_type = strdup(dentry.d_name);
		type_t->so_list = NULL;

		pkg_type_list = g_slist_append(pkg_type_list, (void *)type_t);

		sub_dp = opendir(buf);
		if (sub_dp == NULL) {
			__pkt_type_list_free();
			closedir(dp);
			return AC_R_ERROR;
		}

		while (readdir_r(sub_dp, &sub_dentry, &sub_result) == 0
				&& sub_result != NULL) {
			if (sub_dentry.d_type == DT_DIR)
				continue;
			snprintf(buf2, MAX_LOCAL_BUFSZ, "%s/%s",
					buf, sub_dentry.d_name);
			_D("so_name : %s", buf2);

			handle = dlopen(buf2, RTLD_LAZY);
			if (handle == NULL)
				continue;
			so_t = malloc(sizeof(ac_so_list_t));
			if (so_t == NULL) {
				__pkt_type_list_free();
				dlclose(handle);
				handle = NULL;
				closedir(sub_dp);
				closedir(dp);
				return AC_R_ERROR;
			}
			memset(so_t, 0, sizeof(ac_so_list_t));
			so_t->so_name = strdup(sub_dentry.d_name);
			so_t->ac_check = dlsym(handle, "check_launch_privilege");
			so_t->ac_register = dlsym(handle, "check_register_privilege");
			so_t->ac_unregister = dlsym(handle, "check_unregister_privilege");

			type_t->so_list = g_slist_append(type_t->so_list, (void *)so_t);
			handle = NULL;
		}
		closedir(sub_dp);
	}
	closedir(dp);

	return AC_R_OK;
}

SLPAPI int ac_server_initialize()
{
	int ret = AC_R_OK;

	ret = __initialize();

	return ret;
}

SLPAPI int ac_server_check_launch_privilege(const char *pkg_name, const char *pkg_type, int pid)
{
	int ret = -1;

	ret = __check_launch_privilege(pkg_name, pkg_type, pid);

	return ret;
}
