#!/bin/bash
#SBATCH -J magnetic_recon_bench
#SBATCH -o magnetic_recon_bench.txt
#SBATCH -N 3
#SBATCH -n 3
#SBATCH -p development
#SBATCH -t 00:10:00

OSP_MPI="--osp:mpi"
if [ $SLURM_NNODES -eq 1 ]; then
	echo "Not passing --osp:mpi for single node run"
	OSP_MPI=""
fi

# This camera position is sort of in the back-middle looking over the office interior
CAM_VP="-87.630363 -158.657669 -214.991302"
CAM_VU="0 1 0"
CAM_VI="-88.523651 153.497650 119.189124"
DATA_PATH=/work/03160/will/data

# We take num workers samples per pixel so that's num nodes - 1 to account for the master
((SPP = $SLURM_NNODES - 1))

# Print the expanded list of workers hosts so we can easily paste into osp_separate_launch
#scontrol show hostname $SLURM_NODELIST > stamp-worker-nodes.txt
# Pull the first 3 hosts from SLURM and use them to run lammps
WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "Magnetic Reconnection bench using hosts $WORKER_HOSTS"
set -x
# Note: switch spp to $SLURM_NNODES for the spp scaling runs
ibrun ./ospModelViewer ${DATA_PATH}/magnetic.x3d $OSP_MPI \
	--bench-out magnetic_recon_bench_spp$SLURM_NNODES.ppm \
	--renderer pathtracer --max-accum 1 --accum-reset --nowin --1k \
	--max-depth 5 --spp $SPP --bench 25x50 \
	--hdri-light 0.1 ${DATA_PATH}/campus_probe.pfm \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI

