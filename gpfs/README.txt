This custom metric for Arm Forge Professional measures the average cycles spent in GPFS IOPS and other GPFS metrics.

LICENSE
=======

The code is licensed under the Apache License Version 2.0 -- see LICENSE-2.0.txt for the full text.

PREREQUISITES
=============

The code must be built in a system that has GPFS installed. Check for the /usr/lpp/mmfs/src/include/cxi directory.

INSTALLATION
============

Set ALLINEA_METRIC_PLUGIN_DIR to your Arm Forge Professional Metrics SDK directory, e.g.

export ALLINEA_METRIC_PLUGIN_DIR=$ALLINEA_FORGE_PATH/map/metrics

Then run:

make install
