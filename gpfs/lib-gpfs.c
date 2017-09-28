#include "allinea_metric_plugin_api.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define INCLUDE_PER_CPU_COUNTERS
typedef int Errno;
#include "cxiSharedSeg.h"

#define ERROR_INITIALIZATION_FAILED 100

#define DEV_SS0 "/dev/ss0"

/*! File descriptor for /dev/ss0. */ 
int ss0_fd = -1;

/*! The number of cycles spent in IO at metric initialization. */
static uint64_t cyclesSpentInIOStart;

/*! The number of cycles spent in IO since metric initialization. */
static uint64_t cyclesSpentInIOTotal;

/*! The number of cycles spent in IO this sample. */
static uint64_t cyclesSpentInIOLastSample;

/*! The number of inode lookups at metric initialization. */
static uint64_t inodeLookupsStart;

/*! The number of inode lookups since metric initialization. */
static uint64_t inodeLookupsTotal;

/*! The number of inode lookups this sample. */
static uint64_t inodeLookupsLastSample;

/*! The number of opens at metric initialization. */
static uint64_t opensStart;

/*! The number of opens since metric initialization. */
static uint64_t opensTotal;

/*! The number of opens this sample. */
static uint64_t opensLastSample;

/*! The number of reads at metric initialization. */
static uint64_t readsStart;

/*! The number of reads since metric initialization. */
static uint64_t readsTotal;

/*! The number of reads this sample. */
static uint64_t readsLastSample;

/*! The number of writes at metric initialization. */
static uint64_t writesStart;

/*! The number of writes since metric initialization. */
static uint64_t writesTotal;

/*! The number of writes this sample. */
static uint64_t writesLastSample;

/*! The number of IOPs at metric initialization.  */
static uint64_t iopsStart;

/*! The number of IOPs since metric initialization.  */
static uint64_t iopsTotal;

/*! The number of IOPs this sample.  */
static uint64_t iopsLastSample;

/*! The number of cycles per IOP this sample.  */
static double cyclesPerIOPLastSample;

/*! \a 1 if the initial values of the metrics have not been read yet, else \a 0. */
/*!
 *  The first time \a update is called and this variable is set it will
 *  read the initial values of the metrics and then clear this variable.
 */
static int firstTime = 1;

/*! Time of the last sample. */
/*!
 *  If the time of the current sample is different from the time of the last
 *  then we assume it is a new sample and we need to update our reading of the
 *  counters.
 */
static struct timespec lastSampleTime;

/*! This function is called when the metric plugin is loaded. */
/*!
 *  We do not have to restrict ourselves to async-signal-safe functions because
 *  the initialization function will be called without any locks held.
 * 
 *  \param plugin_id an opaque handle for the plugin.
 *  \param unused unused
 *  \return 0 on success; -1 on failure and set errno
 */
int allinea_plugin_initialize(plugin_id_t plugin_id, void *unused)
{
    (void)unused; /* unused variable */

    ss0_fd = open(DEV_SS0, O_RDONLY);
    if (ss0_fd == -1) {
        int saved_errno = errno;
        if (errno == ENOENT)
            allinea_set_plugin_error_messagef(plugin_id, ERROR_INITIALIZATION_FAILED, "%s: no GPFS filesystem detected", DEV_SS0);
        else
            allinea_set_plugin_error_messagef(plugin_id, ERROR_INITIALIZATION_FAILED, "%s: can't access GPFS shared segment device", DEV_SS0);
        errno = saved_errno;
        return -1;
    }
    firstTime = 1;

    return 0;
}

/*! This function is called when the metric plugin is unloaded. */
/*!
 *  We do not have to restrict ourselves to async-signal-safe functions because
 *  the cleanup function will be called without any locks held.
 * 
 *  \param plugin_id an opaque handle for the plugin.
 *  \param unused unused
 *  \return 0 on success; -1 on failure and set errno
 */
int allinea_plugin_cleanup(plugin_id_t id, void *unused)
{
    (void) id;  // Unused parameter
    (void)unused; /* unused variable */

    if (ss0_fd != -1) {
        close(ss0_fd);
        ss0_fd = -1;
    }
    return 0;
}

/*! Called once per sample to read the metrics from /dev/ss0. */
static int update()
{
    int ret, i;
    uintptr_t args[6];
    PerCpuCounters_t buffer;
    
    if (ss0_fd == -1)
        return 0;

    args[0] = cxiCounterTypeVfsStatsGetAll;
    args[1] = sizeof(PerCpuCounters_t);
    args[2] = (uintptr_t) &buffer;
    ret = ioctl(ss0_fd, GetCounters, args);
    if (ret != 0)
        return -1;
    uint64_t cyclesSpentInIO = 0LL;
    uint64_t iops = 0LL;
    for (i=0;i<nVFSStatItems;++i) {
        cyclesSpentInIO += buffer.vfsstat_count[i].cycles;
        iops += buffer.vfsstat_count[i].count;
    }
    uint64_t inodeLookups = buffer.vfsstat_count[lookupCall].count;
    uint64_t opens = buffer.vfsstat_count[openCall].count;
    uint64_t reads = buffer.vfsstat_count[readCall].count +
                     buffer.vfsstat_count[mmapReadCall].count +
                     buffer.vfsstat_count[aioReadSyncCall].count +
                     buffer.vfsstat_count[aioReadAsyncCall].count;
    uint64_t writes = buffer.vfsstat_count[writeCall].count +
                      buffer.vfsstat_count[mmapWriteCall].count +
                      buffer.vfsstat_count[aioWriteSyncCall].count +
                      buffer.vfsstat_count[aioWriteAsyncCall].count;
    if (firstTime) {
        cyclesSpentInIOStart      = cyclesSpentInIO;
        cyclesSpentInIOLastSample = 0;
        cyclesSpentInIOTotal      = 0;
        inodeLookupsStart         = inodeLookups;
        inodeLookupsLastSample    = 0;
        inodeLookupsTotal         = 0;
        opensStart                = opens;
        opensLastSample           = 0;
        opensTotal                = 0;
        readsStart                = reads;
        readsLastSample           = 0;
        readsTotal                = 0;
        writesStart               = writes;
        writesLastSample          = 0;
        writesTotal               = 0;
        iopsStart                 = iops;
        iopsLastSample            = 0;
        iopsTotal                 = 0;
        cyclesPerIOPLastSample    = 0;

        firstTime                 = 0;
    } else {
        cyclesSpentInIOLastSample = cyclesSpentInIO - cyclesSpentInIOStart - cyclesSpentInIOTotal;
        inodeLookupsLastSample    = inodeLookups    - inodeLookupsStart    - inodeLookupsTotal;
        opensLastSample           = opens  - opensStart  - opensTotal;
        readsLastSample           = reads  - readsStart  - readsTotal;
        writesLastSample          = writes - writesStart - writesTotal;
        iopsLastSample            = iops - iopsStart - iopsTotal;
        cyclesPerIOPLastSample    = (iopsLastSample == 0.0) ? 0.0 : (double) cyclesSpentInIOLastSample / (double) iopsLastSample;
    }
    cyclesSpentInIOTotal = cyclesSpentInIO - cyclesSpentInIOStart;
    inodeLookupsTotal    = inodeLookups    - inodeLookupsStart;
    opensTotal           = opens  - opensStart;
    readsTotal           = reads  - readsStart;
    writesTotal          = writes - writesStart;
    iopsTotal            = iops   - iopsStart;

    return 0;
}

/*! Get the current value of the given metric. */
/*!
 *  \param metricId the ID of the metric to get the value for
 *  \param inCurrentSampleTime [in] the time the metric was sampled
 *  \param inValue pointer to where the metric is stored
 *  \param outValue [out] value will be written here.
 * 
 *  If this is a new sample (\a lastSampleTime != \a inCurrentsampleTime)
 *  then \a update is called to read the latest counters from
 *  /dev/ss0.
 *
 *  \note inValue is a pointer. The value is not read from the pointer until
 *  *after* \a update has been called. \a update may change the pointed to value
 *  which means the contents of \a *inValue may have changed after a call to this
 *  function.
 */    
static int getMetricValue(metric_id_t metricId, const struct timespec *inCurrentSampleTime, uint64_t *inValue, uint64_t *outValue)
{
    (void)metricId; /* unused variable */

    if (ss0_fd == -1)
        return 0;
    
    if (lastSampleTime.tv_sec  != inCurrentSampleTime->tv_sec ||
        lastSampleTime.tv_nsec != inCurrentSampleTime->tv_nsec) {
        lastSampleTime.tv_sec  = inCurrentSampleTime->tv_sec;
        lastSampleTime.tv_nsec = inCurrentSampleTime->tv_nsec;
        int ret = update();
        if (ret != 0)
            return ret;
    }
    
    *outValue = *inValue;
    
    return 0;
}

/*! Get the current value of the given metric. */
/*!
 *  \param metricId the ID of the metric to get the value for
 *  \param inCurrentSampleTime [in] the time the metric was sampled
 *  \param inValue pointer to where the metric is stored
 *  \param outValue [out] value will be written here.
 * 
 *  If this is a new sample (\a lastSampleTime != \a inCurrentsampleTime)
 *  then \a update is called to read the latest counters from
 *  /dev/ss0.
 *
 *  \note inValue is a pointer. The value is not read from the pointer until
 *  *after* \a update has been called. \a update may change the pointed to value
 *  which means the contents of \a *inValue may have changed after a call to this
 *  function.
 */    
static int getMetricValueDouble(metric_id_t metricId, const struct timespec *inCurrentSampleTime, double *inValue, double *outValue)
{
    (void)metricId; /* unused variable */

    if (ss0_fd == -1)
        return 0;
    
    if (lastSampleTime.tv_sec  != inCurrentSampleTime->tv_sec ||
        lastSampleTime.tv_nsec != inCurrentSampleTime->tv_nsec) {
        lastSampleTime.tv_sec  = inCurrentSampleTime->tv_sec;
        lastSampleTime.tv_nsec = inCurrentSampleTime->tv_nsec;
        int ret = update();
        if (ret != 0)
            return ret;
    }
    
    *outValue = *inValue;
    
    return 0;
}

int allinea_gpfsIOCycles(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &cyclesSpentInIOLastSample, outValue);
}

int allinea_gpfsIOCyclesTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &cyclesSpentInIOTotal, outValue);
}

int allinea_gpfsINodeLookups(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &inodeLookupsLastSample, outValue);
}

int allinea_gpfsINodeLookupsTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &inodeLookupsTotal, outValue);
}

int allinea_gpfsOpens(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &opensLastSample, outValue);
}

int allinea_gpfsOpensTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &opensTotal, outValue);
}

int allinea_gpfsReads(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &readsLastSample, outValue);
}

int allinea_gpfsReadsTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &readsTotal, outValue);
}

int allinea_gpfsWrites(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &writesLastSample, outValue);
}

int allinea_gpfsWritesTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &writesTotal, outValue);
}

int allinea_gpfsIOPs(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &iopsLastSample, outValue);
}

int allinea_gpfsIOPsTotal(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, uint64_t *outValue)
{
    return getMetricValue(metricId, inOutCurrentSampleTime, &iopsTotal, outValue);
}

int allinea_gpfsCyclesPerIOP(metric_id_t metricId, struct timespec *inOutCurrentSampleTime, double *outValue)
{
    return getMetricValueDouble(metricId, inOutCurrentSampleTime, &cyclesPerIOPLastSample, outValue);
}
