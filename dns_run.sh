#!/bin/bash

# TODO: Basically copy RM bench script

# TODO: Need better camera position
CAM_VP="2228.15452312142 -1201.71723886076 2055.70343132872"
CAM_VI="-998.143858201647 3175.79946845879 -186.552984667173"
CAM_VU="0.0511834277352562 0.484921924641355 0.873058407970162"

DATA_PATH=/work/03160/will/data

WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "DNS float bench using hosts $WORKER_HOSTS"
set -x
export OSPRAY_DATA_PARALLEL=4x2x2
ibrun ./ospVolumeViewer ${DATA_PATH}/dns_float.osp $OSP_MPI \
	--transferfunction ${DATA_PATH}/carson_dns.tfn \
	--viewsize 1024x1024 \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI


