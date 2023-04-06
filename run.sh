#!/bin/bash

sudo ./pmem_perf_sweep.sh -m ~/projects/mlc/Linux/mlc -p /mnt/pmem1 -s 0 | tee -a PMem_PnP_numa1_single_result.log
