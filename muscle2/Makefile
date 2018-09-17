# Path to the metrics plugin directory. The metric plugin API
# header files should be in the 'include/' subdirectory to this.
ifndef ALLINEA_METRIC_PLUGIN_DIR
$(error "Set ALLINEA_METRIC_PLUGIN_DIR to the Metrics SDK root directory, e.g. $$ALLINEA_FORGE_PATH/map/metrics")
endif

# Path to the MUSCLE2 install directory. 
# Header files should be in the 'include/' subdirectory to this.
# -I/opt/muscle/include/ -L/opt/muscle/lib/ -lmuscle2
ifndef MUSCLE_HOME
$(error "MUSCLE_HOME unset! Make sure to run e.g. 'source /opt/muscle/etc/muscle.profile' before running MUSCLE2.")
endif

CC=gcc
IDIRS=-I ${ALLINEA_METRIC_PLUGIN_DIR}/include -I ${MUSCLE_HOME}/include/muscle2
CFLAGS=-std=gnu99 -Wall -Werror -g
LFLAGS=-fPIC -shared -L${MUSCLE_HOME}/lib -lmuscle2

.PHONY: all
all: libmuscle2.so

libmuscle2.so: libmuscle2.c
	$(CC) $(CFLAGS) $< -o $@ $(IDIRS) $(LFLAGS)

.PHONY: install
install: libmuscle2.so muscle2.xml
	if [ ! -d ~/.allinea/map/metrics ]; then mkdir -p ~/.allinea/map/metrics; fi
	cp -u libmuscle2.so muscle2.xml ~/.allinea/map/metrics/
	if [ ! -d ~/.allinea/perf-report/reports/ ]; then mkdir -p ~/.allinea/perf-report/reports/; fi
	cp -u muscle2-report.xml ~/.allinea/perf-report/reports/

.PHONY: clean
clean:
	rm -f libmuscle2.so
