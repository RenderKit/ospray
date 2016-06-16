#!/bin/bash

DATA_PATH=${WORK}/../data/LoftOffice

OSP_MPI="--osp:mpi"
if [ $SLURM_NNODES -eq 1 ]; then
	echo "Not passing --osp:mpi for single node run"
	OSP_MPI=""
fi

# this is the camera pos from the Embree scene script
CAM_VP="7.63728 -9.89146 3.08021"
CAM_VI="1.4654 -0.462097 3.01829"
CAM_VU="0 -0.0034912 0.999994"

# We take num workers samples per pixel so that's num nodes - 1 to account for the master
((SPP = $SLURM_NNODES - 1))

# Print the expanded list of workers hosts so we can easily paste into osp_separate_launch
#scontrol show hostname $SLURM_NODELIST > stamp-worker-nodes.txt
# Pull the first 3 hosts from SLURM and use them to run lammps
WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "Loft Office bench using hosts $WORKER_HOSTS"
set -x
cd $WORK_DIR
ibrun ./ospModelViewer ${DATA_PATH}/LoftOffice.obj $OSP_MPI \
	--renderer pathtracer --bench-out ${OUT_DIR}/${SLURM_JOB_NAME}.ppm \
	--max-depth 15 --spp $SPP --bench 25x25 --max-accum 1 \
  --accum-reset --nowin --1k \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI \
  --hdri-light 1 ${DATA_PATH}/light_tent_by_lightmapltd.pfm \
  --backplate ${DATA_PATH}/background.ppm

