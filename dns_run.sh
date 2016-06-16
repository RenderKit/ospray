#!/bin/bash

((NUM_BRICKS = $SLURM_NNODES - 1))
if [ $NUM_BRICKS -eq 16 ]; then
  dp_grid=(4 2 2)
elif [ $NUM_BRICKS -eq 32 ]; then
  dp_grid=(4 4 2)
elif [ $NUM_BRICKS -eq 64 ]; then
  dp_grid=(4 4 4)
elif [ $NUM_BRICKS -eq 128 ]; then
  dp_grid=(8 4 4)
fi

CAM_VP="-1.28353 -1.8104 0.26588"
CAM_VI="0.00979014 -0.00798005 -0.0125492"
CAM_VU="0 0 1"

DATA_PATH=/work/03160/will/data

WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "DNS float bench using hosts $WORKER_HOSTS"
echo "Setting OSPRAY_DATA_PARALLEL = ${dp_grid[*]}"
export OSPRAY_DATA_PARALLEL=${dp_grid[0]}x${dp_grid[1]}x${dp_grid[2]}

set -x

ibrun ./ospVolumeViewer ${DATA_PATH}/dns_float.osp --osp:mpi \
	--transferfunction ${DATA_PATH}/carson_dns.tfn \
	--viewsize 1024x1024 \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI \
  --benchmark 25 50 ${OUT_DIR}/$SLURM_JOB_NAME

set +x

