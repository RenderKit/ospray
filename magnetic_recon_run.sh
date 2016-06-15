#!/bin/bash

OSP_MPI="--osp:mpi"
if [ $SLURM_NNODES -eq 1 ]; then
	echo "Not passing --osp:mpi for single node run"
	OSP_MPI=""
fi

# This camera position is sort of in the back-middle looking over the office interior
CAM_VP="-117.255371 -189.252579 -186.258453"
CAM_VU="0 1 0"
CAM_VI="-118.1073 131.917816 139.267548"
DATA_PATH=/work/03160/will/data

# We take num workers samples per pixel so that's num nodes - 1 to account for the master
#((SPP = $SLURM_NNODES - 1))
SPP=256

WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "Magnetic Reconnection bench using hosts $WORKER_HOSTS"
set -x
cd $WORK_DIR
# Note: switch spp to $SLURM_NNODES for the spp scaling runs
ibrun ./ospModelViewer ${DATA_PATH}/magnetic.x3d $OSP_MPI \
	--bench-out ${OUT_DIR}/${SLURM_JOB_NAME}.ppm \
	--renderer pathtracer --max-accum 1 --accum-reset --nowin --1k \
	--max-depth 5 --spp $SPP --bench 25x50 \
	--hdri-light 0.1 ${DATA_PATH}/campus_probe.pfm \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI

