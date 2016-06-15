#!/bin/bash

# TODO: Basically copy RM bench script

CAM_VP="11600.8 13112.7 2607.71"
CAM_VI="5054.02 3055.42 -1012.17"
CAM_VU="0.0511834 0.484922 0.873058"

DATA_PATH=/work/03160/will/data

WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "DNS float bench using hosts $WORKER_HOSTS"
set -x
export OSPRAY_DATA_PARALLEL=4x2x2
ibrun ./ospVolumeViewer ${DATA_PATH}/dns_float.osp --osp:mpi \
	--transferfunction ${DATA_PATH}/carson_dns.tfn \
	--viewsize 1024x1024 \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI \
  --benchmark 1 1 ${OUT_DIR}/$SLURM_JOB_NAME

set +x

