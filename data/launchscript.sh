#!/bin/bash


# ------------------------------------------------------------------
# in case this script get called from within an MPI process we first
# have to unset all these environment variables before we can call
# 'mpirun'; else mpirun gets 'confused'
# ------------------------------------------------------------------

unset I_MPI_PERHOST
unset I_MPI_PM
unset I_MPI_INFO_STATE
unset I_MPI_INFO_MODE
unset I_MPI_INFO_VEND
unset I_MPI_INFO_FLGB
unset I_MPI_INFO_FLGC
unset I_MPI_INFO_FLGD
unset I_MPI_INFO_DESC
unset I_MPI_INFO_BRAND
unset I_MPI_INFO_SERIAL
unset I_MPI_INFO_C_NAME
unset I_MPI_INFO_SIGN
unset I_MPI_INFO_LCPU
unset I_MPI_INFO_PACK
unset I_MPI_INFO_CORE
unset I_MPI_INFO_THREAD
unset I_MPI_INFO_CACHE1
unset I_MPI_INFO_CACHE2
unset I_MPI_INFO_CACHE3
unset I_MPI_INFO_CACHES
unset I_MPI_INFO_CACHE_SHARE
unset I_MPI_INFO_CACHE_SIZE
unset I_MPI_INFO_NC
unset I_MPI_INFO_NP
unset I_MPI_PIN_MAPPING
unset I_MPI_PIN_INFO
unset I_MPI_PIN_DOM
unset MPICH_INTERFACE_HOSTNAME
unset MPICH_ENABLE_CKPOINT
unset PMI_SIZE
unset PMI_RANK
unset HYDI_CONTROL_FD
unset CTL_MODULE_PATH
unset PMI_FD

echo "launching to port "$1

export I_MPI_FABRICS="tcp:tcp"

HOSTFILENAME=/tmp/.ospray.$USER
rm $HOSTFILENAME
echo mic0 >> $HOSTFILENAME
#mpirun -perhost 1 -np 1 -f $HOSTFILENAME ./ospray_mpi_worker --osp:connect $1 > /tmp/launch.out 
#export I_MPI_FABRICS=tmi
mpirun \
 -genv I_MPI_DEBUG 5 \
 -perhost 1 -np 2 -hostfile $HOSTFILENAME ./ospray_mpi_worker --osp:connect $1 > /tmp/ospray.launch.$user.out 
#mpiexec.hydra -np 2 ./ospray_mpi_worker > /tmp/launch.out &






