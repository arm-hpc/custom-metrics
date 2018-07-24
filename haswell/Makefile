# Copyright (c) 2018, Arm Limited and affiliates.
# SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Only want to check variables if we are not performing a clean
ifneq ($(MAKECMDGOALS),clean)
# Path to the metrics plugin directory. The metric plugin API
# header files should be in the 'include/' subdirectory to this.
ifndef ARM_FORGE_METRIC_PLUGIN_DIR
$(error "Set ARM_FORGE_METRIC_PLUGIN_DIR to the directory containg the Arm Metrics SDK headers. This is typically <Arm MAP install dir>/map/metrics.")
endif

ifndef CXX
$(warning "CXX not set. Setting C++ compiler to g++")
CXX=g++
endif
endif

#PAPI_DIR=/path/to/papi/installation
PAPI_DIR=/usr

CFLAGS=--std=c++11 -O3 -fPIC -I$(ARM_FORGE_METRIC_PLUGIN_DIR)/include -I$(PAPI_DIR)/include
LFLAGS=-L$(PAPI_DIR)/lib -lpapi
DEFAULTCONFIGDIR=~/.allinea/map/metrics

CONFIGDIR := $(shell if [ -z "${ALLINEA_CONFIG_DIR}" ]; then echo "$(DEFAULTCONFIGDIR)"; else echo "${ALLINEA_CONFIG_DIR}/map/metrics";  fi)

.PHONY: all
all: libhaswellmemorybound.so
	@echo "Use 'make install' to install the metric to $(CONFIGDIR) for testing."

libhaswellmemorybound.so: lib_haswell_memory_bound.cpp
	$(CXX) $(CFLAGS) -shared -o $@ $< $(LFLAGS)

.PHONY: install
install: libhaswellmemorybound.so haswell_memory_bound.xml
	if [ ! -d $(CONFIGDIR) ]; then mkdir -p $(CONFIGDIR); fi
	cp -u $^ $(CONFIGDIR)

.PHONY: clean
clean:
	rm -f libhaswellmemorybound.so
