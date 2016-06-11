#!/bin/bash

OSP_MPI="--osp:mpi"
if [ $SLURM_NNODES -eq 1 ]; then
	echo "Not passing --osp:mpi for single node run"
	OSP_MPI=""
fi

# TODO: Need better camera position
CAM_VP="-2469.9 -2302.92 1035.65"
CAM_VI="5284.37 4329.21 664.905"
CAM_VU="0 0 1"
DATA_PATH=/work/03160/will/data

# Print the expanded list of workers hosts so we can easily paste into osp_separate_launch
#scontrol show hostname $SLURM_NODELIST > stamp-worker-nodes.txt
# Pull the first 3 hosts from SLURM and use them to run lammps
WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "DNS float bench using hosts $WORKER_HOSTS"
set -x
export OSPRAY_DATA_PARALLEL=4x4x4
ibrun ./ospVolumeViewer ${DATA_PATH}/dns_float.osp $OSP_MPI \
	--transferfunction ${DATA_PATH}/dns_float_transfer_fcn.tfn \
	--benchmark 25 50 dns_bench$SLURM_NNODES --viewsize 1024x1024 \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI


