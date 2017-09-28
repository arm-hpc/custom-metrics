# Path to the metrics plugin directory. The metric plugin API
# header files should be in the 'include/' subdirectory to this.
ifndef ALLINEA_METRIC_PLUGIN_DIR
$(error "Set ALLINEA_METRIC_PLUGIN_DIR to the Metrics SDK root directory, e.g. $$ALLINEA_FORGE_PATH/map/metrics")
endif
ALLINEA_METRIC_INSTALL_DIR=~/.allinea/map/metrics

UNAME := $(shell uname -m)

ifeq ($(UNAME),x86_64)
GPFS_ARCH=GPFS_ARCH_X86_64
else ifeq ($(UNAME),ppc64)
GPFS_ARCH=GPFS_ARCH_PPC64
else ifeq ($(UNAME),ppc64le)
GPFS_ARCH=GPFS_ARCH_PPC64_LE
else
$(error GPFS does not support your architecture: $(UNAME))
endif

CC=gcc
CFLAGS=-D_REENTRANT -D$(GPFS_ARCH) -I/usr/lpp/mmfs/src/include/cxi -I${ALLINEA_METRIC_PLUGIN_DIR}/include -Wall -Werror -Wno-attributes -fno-omit-frame-pointer -g -Wno-unused-but-set-variable
LFLAGS=-fPIC -shared
WRAP_LFLAGS=-Wl,--wrap=open -Wl,--wrap=ioctl -Wl,--wrap=close

.PHONY: all
all: lib-gpfs.so gpfs-test
	@echo "Use make install to install the metric in ${ALLINEA_METRIC_INSTALL_DIR} for testing."

lib-gpfs.so: lib-gpfs.c
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)

gpfs-test: gpfs-test.c lib-gpfs.c
	$(CC) $(CFLAGS) gpfs-test.c -c
	$(CC) $(CFLAGS) lib-gpfs.c  -c
	$(CC) $(CFLAGS) gpfs-test.o lib-gpfs.o -o $@ $(WRAP_LFLAGS)

.PHONE: test
test: gpfs-test
	./gpfs-test

.PHONY: install
install: lib-gpfs.so gpfs.xml
	if [ ! -d ${ALLINEA_METRIC_INSTALL_DIR} ]; then mkdir -p ${ALLINEA_METRIC_INSTALL_DIR}; fi
	cp -u lib-gpfs.so gpfs.xml ${ALLINEA_METRIC_INSTALL_DIR}

.PHONY: clean
clean:
	rm -f lib-gpfs.so gpfs-test.o lib-gpfs.o gpfs-test
