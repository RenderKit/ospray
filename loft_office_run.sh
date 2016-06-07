#!/bin/bash
#SBATCH -J loft_office_bench
#SBATCH -o loft_office_bench.txt
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
CAM_VP="3.373260 -9.858215 6.385623"
CAM_VU="0.0 1.0 0.0"
CAM_VI="3.401789 -0.356800 3.917832"

# Print the expanded list of workers hosts so we can easily paste into osp_separate_launch
#scontrol show hostname $SLURM_NODELIST > stamp-worker-nodes.txt
# Pull the first 3 hosts from SLURM and use them to run lammps
WORKER_HOSTS=`scontrol show hostname $SLURM_NODELIST | tr '\n' ',' | sed s/,$//`
echo "Loft Office bench using hosts $WORKER_HOSTS"
set -x
ibrun ./ospModelViewer /work/03160/will/data/LoftOffice/LoftOffice.obj $OSP_MPI \
	--renderer pathtracer --bench-out loft_office_spp_scale$SLURM_NNODES.ppm \
	--max-depth 15 --spp $SLURM_NNODES --bench 25x25 --max-accum 1 --accum-reset --nowin --1k \
	-vp $CAM_VP -vu $CAM_VU -vi $CAM_VI

