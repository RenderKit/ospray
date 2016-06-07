#!/bin/bash

# Trying minimal number of bricks for the nodes so each node gets a single brick
# The data with this bricking is in the rm_min_brick* runs
dp_grids[2]=1x1x1
dp_grids[3]=2x1x1
dp_grids[5]=2x2x1
dp_grids[9]=2x2x2
dp_grids[17]=4x2x2
dp_grids[33]=4x4x2
dp_grids[65]=4x4x4

OSP_MPI="--osp:mpi"
OSP_VIEWER_CMD="ibrun ./ospVolumeViewer"
if [ $SLURM_NNODES -eq 1 ]; then
	echo "Not passing --osp:mpi for single node run"
	OSP_MPI=""
  OSP_VIEWER_CMD="./ospVolumeViewer"
	unset OSPRAY_DATA_PARALLEL
else
  set -x
  #echo "Not setting OSPRAY_DATA_PARALLEL for replicated data benchmark"
  #unset OSPRAY_DATA_PARALLEL
  # For min brick or adaptive brick runs
  export OSPRAY_DATA_PARALLEL=${dp_grids[$SLURM_NNODES]}
  # For fixed brick runs
  #export OSPRAY_DATA_PARALLEL=4x4x4
  set +x
fi

# 1.75^3 scales the volume up to ~43GB
# 1.6^3 gives ~39GB per node
rm_scale=(1.75 1.75 1.75)
export OSPRAY_RM_SCALE_FACTOR=${rm_scale[0]}x${rm_scale[1]}x${rm_scale[2]}

DATA_PATH=/work/03160/will/data

# Print the expanded list of workers hosts so we can easily paste into osp_separate_launch
#scontrol show hostname $SLURM_NODELIST > stamp-worker-nodes.txt
# Pull the first 3 hosts from SLURM and use them to run lammps
WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "RM bench using hosts $WORKER_HOSTS"

echo "Sleeping for 10s to make sure X-server is running"
# Log the command used to run the benchmark
set -x

sleep 10

${OSP_VIEWER_CMD} ${DATA_PATH}/bob273.bob $OSP_MPI --viewsize 1024x1024 \
	--transferfunction ${DATA_PATH}/rm_more_transparent.tfn \
	--benchmark 25 50 $SLURM_JOB_NAME \
  -vp "0 0.001 2.4" -vi "0 0 0"

set +x

