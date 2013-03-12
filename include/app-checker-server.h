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


#ifndef __APP_CHECKER_SERVER_H__
#define __APP_CHECKER_SERVER_H__

#ifdef __cpulusplus
extern "C" {
#endif

typedef enum _ac_return_val {
	AC_R_ERROR = -1,		/**< General error */
	AC_R_OK = 0			/**< General success */
}ac_return_val;

int ac_server_initialize();
int ac_server_check_launch_privilege(const char *pkg_name, const char *pkg_type, int pid);

#ifdef __cpulusplus
	}
#endif

#endif		/* __APP_CHECKER_SERVER_H__ */




