MUSCLE2 CUSTOM METRIC
=====================


This project is a custom metric for Arm Forge Professional (MAP) to record the communication metrics for the MUSCLE2 library.
This metric is designed for use within the ComPat project.

LICENSE
=======

The code is licensed under the Apache License Version 2.0 -- see LICENSE-2.0.txt for the full text.


USAGE
=====


This custom metric depends on a modified version of MUSCLE2, [GitHub MUSCLE2](https://github.com/psnc-apps/muscle2) from the compat branch-- included as a submodule.
Checkout the project with the `--recursive` flag and build the source code in the muscle2 folder.

Update the Makefile to point to your correct map/metrics install directory, then run:
`make clean all install`

This should place the newly built metric in your local metrics dir `~/.allinea/map/metrics`.

Now when you run a MAP profile it should collect the extra MUSCLE2 communication data.

Note this metric also contains an Arm Performance Reports Partial Reports, which will be installed by default, for presenting MUSCLE2 data in Performance Reports.


POC
===

Oliver Perks - <olly.perks@arm.com>

ComPat H2020 Project - No. 671564
