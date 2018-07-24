#!/bin/bash

output=merged.json

file1="stalls_1.json"
file2="stalls_2.json"

# include the info object from stalls_1.json
jq '{ info,
      samples: {
        metrics : .samples.metrics|with_entries(select(.key|
                contains("haswell.papi.active_cycles")
                or contains("haswell.papi.productive_cycles")
                or contains("haswell.papi.stall_cycles")
                or contains("haswell.papi.store_buffer_stall_cycles")
                or contains("haswell.papi.l1d_pending_stall_cycles")
                or contains("haswell.papi.memory_bound")
                                                ))}}' $file1 > tmp1

# include the info object from stalls_2.json
jq '{ samples: {
        metrics : .samples.metrics|with_entries(select(.key|
                contains("haswell.papi.l1d_pend_miss_fb_full_cycles")
                or contains("haswell.papi.offcore_requests_buffer_sq_cycles")
                or contains("haswell.papi.bandwidth_bound")
                                                ))}}' $file2 > tmp2

# merge the files
jq -n 'reduce inputs as $i ({}; . * $i)' tmp1 tmp2 > $output

# clean up
rm tmp1 tmp2
