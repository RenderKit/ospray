#!/bin/bash

# Trying minimal number of bricks for the nodes so each node gets a single brick
# The data with this bricking is in the rm_min_brick* runs
MIN_BRICK=1

OSP_MPI="--osp:mpi"
OSP_VIEWER_CMD="ibrun ./ospVolumeViewer"

if [ $SLURM_NNODES -eq 1 ]; then
	echo "Not passing --osp:mpi for single node run"
	OSP_MPI=""
  OSP_VIEWER_CMD="./ospVolumeViewer"
	unset OSPRAY_DATA_PARALLEL
elif [ $MIN_BRICK -eq 1 ]; then
  echo "Setting up data parallel grid for min brick run"
  if [ $SLURM_NNODES -eq 2 ]; then
    dp_grid=(1 1 1)
  elif [ $SLURM_NNODES -le 9 ]; then
    dp_grid=(2 2 2)
  elif [ $SLURM_NNODES -le 27 ]; then
    dp_grid=(3 3 3)
  elif [ $SLURM_NNODES -le 64 ]; then
    dp_grid=(4 4 4)
  elif [ $SLURM_NNODES -le 125 ]; then
    dp_grid=(5 5 5)
  fi
else
  echo "Setting up data parallel grid for fixed 4^3 brick run"
  dp_grid=(4 4 4)
fi

# Set the data scaling ratio for this node count
if [ $SLURM_NNODES -eq 2 ]; then
  scale_factor=(1 1 1)
elif [ $SLURM_NNODES -eq 3 ]; then
  scale_factor=(2 1 1)
elif [ $SLURM_NNODES -eq 5 ]; then
  scale_factor=(2 2 1)
elif [ $SLURM_NNODES -eq 9 ]; then
  scale_factor=(2 2 2)
elif [ $SLURM_NNODES -eq 17 ]; then
  scale_factor=(4 2 2)
elif [ $SLURM_NNODES -eq 33 ]; then
  scale_factor=(4 4 2)
elif [ $SLURM_NNODES -eq 65 ]; then
  scale_factor=(4 4 4)
fi

echo "DP Grid ${dp_grid[*]}"
echo "Weak Scale Factor ${scale_factor[*]}"

set -x
#echo "Not setting OSPRAY_DATA_PARALLEL for replicated data benchmark"
#unset OSPRAY_DATA_PARALLEL
# For min brick or adaptive brick runs
export OSPRAY_DATA_PARALLEL=${dp_grid[0]}x${dp_grid[1]}x${dp_grid[2]}
set +x

# 1.75^3 scales the volume up to ~43GB
# 1.6^3 gives ~39GB per node
rm_scale=(1.75 1.75 1.75)
rm_scale[0]=`echo "${rm_scale[0]} * ${scale_factor[0]}" | bc`
rm_scale[1]=`echo "${rm_scale[1]} * ${scale_factor[1]}" | bc`
rm_scale[2]=`echo "${rm_scale[2]} * ${scale_factor[2]}" | bc`
export OSPRAY_RM_SCALE_FACTOR=${rm_scale[0]}x${rm_scale[1]}x${rm_scale[2]}
echo "RM Scaling ${rm_scale[*]}"

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
  -vp "0 0.001 2.4" -vi "0 0 0" --osp:numthreads 22

set +x

