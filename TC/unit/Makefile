CC ?= gcc

TARGETS = utc_ApplicationFW_ac_register_launch_privilege_func \
	utc_ApplicationFW_ac_unregister_launch_privilege_func \
	utc_ApplicationFW_ac_check_launch_privilege_func

PKGS = glib-2.0 dlog app-checker

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s
LDFLAGS += -L/usr/lib -lpthread

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)
