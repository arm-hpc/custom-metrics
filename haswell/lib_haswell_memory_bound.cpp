/*
 * Copyright (c) 2018, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// The next include is required to create a custom metric for Arm MAP
#include "allinea_metric_plugin_api.h"
#include "papi.h"

#include <cstdlib>
#include <sys/syscall.h>
#include <unistd.h>
#include <array>
#include <cassert>
#include <algorithm>

#define ONE_SECOND_NS      1000000000   // The number of nanoseconds in one second

static const int ERROR = -1; // Returned by a function when there is an error

///////////////////////////////////////////////////////////////////////////////
// The next definitions are user defined. We know the names of the counters
// that are to be collected, and assign each of these an index into an array of
// values. EventInds is an enumeration of indices; gEventNames is the list of
// string names of the counters (and masks) that we want to collect;
// gEventCodes stores the codes obtained for the string identifiers of the
// events; and gEventValues is the array into which the counter values are put
///////////////////////////////////////////////////////////////////////////////

// We know the names of the events that we want
enum EventInds {
    CLK_UNHALTED_IND=0,
    CYCLE_ACTIVITY_NO_EXECUTE_IND,
    RESOURCE_STALLS_SB_IND,
    CYCLE_ACTIVITY_STALLS_L1D_PENDING_IND,
    L1D_PEND_MISS_FB_FULL_IND,
    OFFCORE_REQUESTS_BUFFER_SQ_IND,
    NUM_INDS
};

constexpr static std::array<const char*, EventInds::NUM_INDS> gEventNames {
    "CPU_CLK_UNHALTED",
    "CYCLE_ACTIVITY:CYCLES_NO_EXECUTE",
    "RESOURCE_STALLS:SB",
    "CYCLE_ACTIVITY:STALLS_L1D_PENDING",
    "L1D_PEND_MISS:FB_FULL",
    "OFFCORE_REQUESTS_BUFFER:SQ_FULL"
};

// We want to store the event codes, but don't necessarily know these (we can
// get them from the documentation for a particular hardware set, or we can let
// PAPI return the code for the string name during set up)
static std::array<int, EventInds::NUM_INDS> gEventCodes;

// Want to also store the event values
static std::array<long long, EventInds::NUM_INDS> gEventValues;

// A global PAPI event set is stored to collect the counter values
static int gEventSet= PAPI_NULL;

// Forward declaration. Used so that in this section we can have all of the
// functions that are required to report the data for MAP
static int update_values(metric_id_t metric_id, const struct timespec* current_sample_time);

extern "C"{
/**
 * Sets the number of active cycles that have been recorded since the last
 * sample
 *
 * \param [in] metric_id This is required by the MAP tool to identify the
 *                       metric being collected. This can mostly be ignored,
 *                       except when reporting an error back to Arm MAP.
 * \param [in] current_sample_time The time at which the sample is taken. This
 *                                 is passed in from the Arm MAP sampler.
 *                                 Whilst sampling, the same sample time is
 *                                 passed to all of the metric collection
 *                                 functions. This means that it is possible to
 *                                 collect multiple items of data from a common
 *                                 source only once. In this file we collect
 *                                 all of the counter values only once by
 *                                 checking if the current time has been
 *                                 encountered before
 * \param [out] out_value The value of the counters at the given sample time.
 *                        This is the value that will be reported in the Arm
 *                        MAP front end.
 */
int haswell_membound_active_cycles(metric_id_t metric_id,
        struct timespec *current_sample_time, uint64_t *out_value)
{
    // Update the counter values at the current sample period
    update_values(metric_id, current_sample_time);

    *out_value= gEventValues.at(EventInds::CLK_UNHALTED_IND);
    return 0;
}

int haswell_membound_productive_cycles(metric_id_t metric_id,
        struct timespec *current_sample_time, double* out_value)
{
    // Update the counter values at the current sample period
    update_values(metric_id, current_sample_time);

    // The value out here is given as a fraction of active cycles
    *out_value= static_cast<double>(gEventValues.at(EventInds::CLK_UNHALTED_IND) -
            gEventValues.at(EventInds::CYCLE_ACTIVITY_NO_EXECUTE_IND)) /
        gEventValues.at(EventInds::CLK_UNHALTED_IND);
    return 0;
}

int haswell_membound_stall_cycles(metric_id_t metric_id,
        struct timespec *current_sample_time, double *out_value)
{
    // Update the counter values at the current sample period
    update_values(metric_id, current_sample_time);

    // The value out here is given as a fraction of active cycles
    *out_value= static_cast<double>(gEventValues.at(EventInds::CYCLE_ACTIVITY_NO_EXECUTE_IND)) /
        gEventValues.at(EventInds::CLK_UNHALTED_IND);

    return 0;
}

static uint64_t memory_bound_measure()
{
    return std::max(gEventValues.at(EventInds::RESOURCE_STALLS_SB_IND),
            gEventValues.at(EventInds::CYCLE_ACTIVITY_STALLS_L1D_PENDING_IND));
}

int haswell_membound_memory_bound(metric_id_t metric_id,
        struct timespec *current_sample_time, double *out_value)
{
    // Update the counter values at the current sample period
    update_values(metric_id, current_sample_time);

    // The value out here is given as a fraction of active cycles
    *out_value= static_cast<double>(memory_bound_measure()) /
        gEventValues.at(EventInds::CLK_UNHALTED_IND);
    return 0;
}

static uint64_t bandwidth_bound_measure()
{
    return std::max(gEventValues.at(EventInds::RESOURCE_STALLS_SB_IND),
            gEventValues.at(EventInds::L1D_PEND_MISS_FB_FULL_IND) +
            gEventValues.at(EventInds::OFFCORE_REQUESTS_BUFFER_SQ_IND));
}

int haswell_membound_bandwidth_bound(metric_id_t metric_id,
        struct timespec *current_sample_time, double *out_value)
{
    // Update the counter values at the current sample period
    update_values(metric_id, current_sample_time);

    // The value out here is given as a fraction of active cycles
    *out_value= static_cast<double>(bandwidth_bound_measure()) /
        gEventValues.at(EventInds::CLK_UNHALTED_IND);
    return 0;
}

int haswell_membound_latency_bound(metric_id_t metric_id,
        struct timespec *current_sample_time, double *out_value)
{
    // Update the counter values at the current sample period
    update_values(metric_id, current_sample_time);

    // The value out here is given as a fraction of active cycles
    *out_value= static_cast<double>(memory_bound_measure() - bandwidth_bound_measure()) /
        gEventValues.at(EventInds::CLK_UNHALTED_IND);

    return 0;
}

int haswell_membound_other_stall_reason(metric_id_t metric_id,
        struct timespec *current_sample_time, uint64_t *out_value)
{
    // Update the counter values at the current sample period
    update_values(metric_id, current_sample_time);

    // The value out here is given as a fraction of active cycles
    *out_value= static_cast<double>(gEventValues.at(EventInds::CYCLE_ACTIVITY_NO_EXECUTE_IND) -
            memory_bound_measure()) /
        gEventValues.at(EventInds::CLK_UNHALTED_IND);

    return 0;
}
} // extern "C"

//! Returns the thread id of the calling thread
static unsigned long int haswell_membound_get_thread_id()
{
    return syscall(__NR_gettid);
}


/**
 * Initialises the PAPI library and adds the counters defined at the beginning
 * of the file to an event set. It is assumed in this simple implementation
 * that the names that are given for the counters are correct. The error
 * checking in here is not complete.
 */
int haswell_membound_initialise_papi(plugin_id_t plugin_id)
{
    // Initialise the library and check the initialisation was successful
    int retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT  &&  retval > 0)
    {
        allinea_set_plugin_error_messagef(plugin_id, retval, "PAPI library version mismatch. PAPI error: %s", PAPI_strerror(retval));
        return ERROR;
    }
    if (retval < 0)
    {
        allinea_set_plugin_error_messagef(plugin_id, retval, "Could not initialise PAPI library. PAPI error: %s", PAPI_strerror(retval));
        return ERROR;
    }
    retval = PAPI_is_initialized();
    if (retval != PAPI_LOW_LEVEL_INITED)
    {
        allinea_set_plugin_error_messagef(plugin_id, retval, "PAPI incorrectly initialised. PAPI error: %s", PAPI_strerror(retval));
        return ERROR;
    }
    // Initialise thread support (as the program being profiled may be multithreaded).
    retval = PAPI_thread_init(haswell_membound_get_thread_id);
    if (retval != PAPI_VER_CURRENT  &&  retval > 0)
    {
        allinea_set_plugin_error_messagef(plugin_id, retval, "Could not enable thread support (error in PAPI_thread_init). PAPI error: %s", PAPI_strerror(retval));
        return ERROR;
    }
    retval = PAPI_is_initialized();
    if (retval != PAPI_THREAD_LEVEL_INITED+PAPI_LOW_LEVEL_INITED)
    {
        allinea_set_plugin_error_messagef(plugin_id, retval, "PAPI not initialised with thread support. PAPI error: %s", PAPI_strerror(retval));
        return ERROR;
    }

    int maxHardwareCounters = PAPI_num_counters();
    if (maxHardwareCounters < 0)
    {
        allinea_set_plugin_error_messagef(plugin_id, maxHardwareCounters, "This installation does not support PAPI");
        return ERROR;
    }
    else if (maxHardwareCounters == 0)
    {
        allinea_set_plugin_error_messagef(plugin_id, 0, "This machine does not provide hardware counters");
        return ERROR;
    }

    // Get the event codes for the string descriptors
    // Begin by initialising the event codes to some known invalid value
    gEventCodes.fill(0);
    auto codeIt= gEventCodes.begin();
    // At this point the event codes array should be the same size as the event
    // names array
    assert(gEventCodes.size() == gEventNames.size());
    for (const auto& name : gEventNames) {
        int eventCode= PAPI_NULL;
        if (PAPI_event_name_to_code(const_cast<char*>(name), &eventCode) != PAPI_OK) {
            //  TODO: Add some actual error handling in here. For the time
            //  being we just ignore any issue with the counter name
            codeIt++;
            continue;
        }
        *codeIt= eventCode;
        codeIt++;
    }

    return 0;
}

extern "C" {
    // This function is called before the program starts executing. The function
    // signature must remain unchanged to be picked up by the Arm MAP sampler. This is where initialization of the PAPI library and the
    int allinea_plugin_initialize(plugin_id_t plugin_id, void *unused)
    {
        if (haswell_membound_initialise_papi(plugin_id) != 0)
        {
            // allinea_set_plugin_error_message() should have been called by haswell_membound_initialise_papi()
            return ERROR;
        }

        // Create the event sets
        int retval = PAPI_create_eventset(&gEventSet);
        if (retval != PAPI_OK)
        {
            allinea_set_plugin_error_messagef(plugin_id, retval, "Could not create event set: %s", PAPI_strerror(retval));
            return ERROR;
        }

        // We assume that all of the events have been found at this point
        retval= PAPI_add_events(gEventSet, gEventCodes.data(), gEventCodes.size());
        if (retval != PAPI_OK) {
            if (retval > 0){
                allinea_set_plugin_error_messagef(plugin_id, retval, "Error adding"
                        " events to the event set. First error detected adding "
                        "event \"%s\". PAPI error string given as  %s", 
                        gEventNames[retval-1], PAPI_strerror(retval));
            }
            else {
                allinea_set_plugin_error_messagef(plugin_id, retval, "Error "
                        "adding events to the event set: %s",
                        PAPI_strerror(retval));
            }
            return ERROR;
        }

        // Start the event set
        retval = PAPI_start(gEventSet);
        if (retval != PAPI_OK)
        {
            allinea_set_plugin_error_messagef(plugin_id, retval, "Could not get PAPI_start: %s", PAPI_strerror(retval));
            return retval;
        }

        // Set the counter values to zero to start with
        gEventValues.fill(0);

        return 0;
    }


    // This method is called after the main application has finished. This cleans
    // up and stops PAPI from collecting metrics
    int allinea_plugin_cleanup(plugin_id_t plugin_id, void *unused)
    {
        // Stop the event set counting
        int retval= PAPI_stop(gEventSet, gEventValues.data());
        if (retval != PAPI_OK)
        {
            allinea_set_plugin_error_messagef(plugin_id, retval, "Error in PAPI_stop: %s", PAPI_strerror(retval));
            return ERROR;
        }
        // Remove all events from the event set
        retval = PAPI_cleanup_eventset(gEventSet);
        if (retval != PAPI_OK)
        {
            allinea_set_plugin_error_messagef(plugin_id, retval, "Error in PAPI_cleanup_eventset: %s", PAPI_strerror(retval));
            return ERROR;
        }

        // Destroy the event set
        retval = PAPI_destroy_eventset(&gEventSet);
        if (retval != PAPI_OK)
        {
            allinea_set_plugin_error_messagef(plugin_id, retval, "Error in PAPI_destroy_eventset: %s", PAPI_strerror(retval));
            return ERROR;
        }

        // Reset the event set
        gEventSet= PAPI_NULL;

        return 0;
    }

} // extern "C"

// The following function, during sample time, will update the counter values
// stored. This uses PAPI_accum, which resets the counter values after reading
// them
static int update_values(metric_id_t metric_id, const struct timespec* current_sample_time)
{
    static std::uint_fast64_t sLastSampleTime= 0;
    const std::uint_fast64_t now= current_sample_time->tv_nsec + current_sample_time->tv_sec * ONE_SECOND_NS;
    // If we have already updated for the current sample there is nothing to do
    if (now == sLastSampleTime)
        return 0;

    // Accumulate the values in the counters. The counter values are zeroed
    // before this method, and counters are reset after retrieving the value
    gEventValues.fill(0);
    int retval= PAPI_accum(gEventSet, gEventValues.data());
    if (retval != PAPI_OK){
        allinea_set_metric_error_messagef(metric_id, retval, "Error updating metric values: %s", PAPI_strerror(retval));
        return ERROR;
    }

    sLastSampleTime= now;
    return 0;
}
