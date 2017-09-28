#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "allinea_metric_plugin_api.h"

#define DEV_SS0 "/dev/ss0"

static const char *ss0_dat_filename;
static int dev_ss0_fd;

extern int __real_ioctl(int fd, unsigned long request, ...);
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    uintptr_t *args;
    va_list ap;

    va_start(ap, request);
    args = va_arg(ap, void *);
    
    if (fd == dev_ss0_fd) {
        int actual_type =    (int) args[0];
        (void) actual_type;
        size_t actual_size = (size_t) args[1];
        void *buffer =       (void *) args[2];    
    
        FILE *fh = fopen(ss0_dat_filename, "r");
        fseek(fh, 0, SEEK_END);
        long expected_size = ftell(fh);
        fseek(fh, 0, SEEK_SET);
        if (expected_size != actual_size) {
            fprintf(stderr, "FAIL: ioctl: expected size %ld != actual size %ld\n", expected_size, (long) actual_size);
            abort();
            errno = EINVAL;
            return -1;
        }
        size_t bytes_read = fread(buffer, 1, expected_size, fh);
        if (bytes_read != expected_size) {
            fprintf(stderr, "FAIL: fread: expected size %ld != bytes read %ld\n", expected_size, (long) bytes_read);
            abort();
            errno = EINVAL;
            return -1;
        }
        fclose(fh);
        return 0;
    }
    va_end(ap);
    
    __real_ioctl(fd, request, args);    
    return 0;
}

extern int __real_open(const char *pathname, int flags);
int __wrap_open(const char *pathname, int flags)
{
    if (strcmp(pathname, DEV_SS0) == 0) {
        if (flags != O_RDONLY) {
            fprintf(stderr, "FAIL: open: expected flags O_RDONLY != actual_flags: %d\n", flags);
            abort();
            errno = EINVAL;
            return -1;
        }
        dev_ss0_fd = __real_open("/dev/null", flags);
        return dev_ss0_fd;
    }
    return __real_open(pathname, flags);
}

extern int __real_close(int fd);
int __wrap_close(int fd)
{
    if (fd == dev_ss0_fd)
        return 0;
    return __real_close(fd);
}

void allinea_set_plugin_error_messagef(plugin_id_t id, int error_code, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

extern int allinea_plugin_initialize(plugin_id_t plugin_id, void *unused);
extern int allinea_plugin_cleanup(plugin_id_t id, void *unused);
extern int allinea_gpfsIOCycles(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue);
extern int allinea_gpfsIOCyclesTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue);
extern int allinea_gpfsOpens(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue);
extern int allinea_gpfsOpensTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue);
extern int allinea_gpfsINodeLookups(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue);
extern int allinea_gpfsINodeLookupsTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue);

int main(void)
{
    int ret;
    struct timespec sampleTime;
    uint64_t value;

    ret = allinea_plugin_initialize(1, NULL);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_plugin_initialize: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }

    ss0_dat_filename = "ss0.dat.0";

    sampleTime.tv_sec = 1;
    sampleTime.tv_nsec = 0;

    ret = allinea_gpfsIOCycles(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCycles: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 0LL) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCycles: expected 0 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsIOCyclesTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCyclesTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 0LL) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCyclesTotal: expected 0 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsOpens(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsOpens: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 0LL) {
        fprintf(stderr, "FAIL: allinea_gpfsOpens: expected 0 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsOpensTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsOpensTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 0LL) {
        fprintf(stderr, "FAIL: allinea_gpfsOpensTotal: expected 0 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsINodeLookups(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookups: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 0LL) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookups: expected 0 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsINodeLookupsTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookupsTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 0LL) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookupsTotal: expected 0 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }

    ss0_dat_filename = "ss0.dat.1";

    sampleTime.tv_sec = 2;
    sampleTime.tv_nsec = 0;

    ret = allinea_gpfsIOCycles(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCycles: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 58424594407LL) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCycles: expected 58424594407 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsIOCyclesTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCyclesTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 58424594407LL) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCyclesTotal: expected 58424594407 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsOpens(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsOpens: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 340LL) {
        fprintf(stderr, "FAIL: allinea_gpfsOpens: expected 340 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsOpensTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsOpensTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 340LL) {
        fprintf(stderr, "FAIL: allinea_gpfsOpensTotal: expected 340 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsINodeLookups(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookups: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 9LL) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookups: expected 9 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsINodeLookupsTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookupsTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 9LL) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookupsTotal: expected 9 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }

    ss0_dat_filename = "ss0.dat.2";

    sampleTime.tv_sec = 3;
    sampleTime.tv_nsec = 0;

    ret = allinea_gpfsIOCycles(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCycles: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 939585369130LL) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCycles: expected 939585369130 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsIOCyclesTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCyclesTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 998009963537LL) {
        fprintf(stderr, "FAIL: allinea_gpfsIOCyclesTotal: expected 998009963537 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsOpens(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsOpens: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 3978LL) {
        fprintf(stderr, "FAIL: allinea_gpfsOpens: expected 3978 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsOpensTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsOpensTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 4318LL) {
        fprintf(stderr, "FAIL: allinea_gpfsOpensTotal: expected 4318 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsINodeLookups(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookups: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 161LL) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookups: expected 161 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    allinea_gpfsINodeLookupsTotal(1, &sampleTime, &value);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookupsTotal: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    if (value != 170LL) {
        fprintf(stderr, "FAIL: allinea_gpfsINodeLookupsTotal: expected 170 != actual %llu\n", (unsigned long long) value);
        abort();
        return 1;
    }
    
    ret = allinea_plugin_cleanup(1, NULL);
    if (ret != 0) {
        fprintf(stderr, "FAIL: allinea_plugin_initialize: failed with return value %d errno %d (%s)\n", ret, errno, strerror(errno));
        abort();
        return 1;
    }
    
    fprintf(stderr, "PASS\n");
    
    return 0;
}
