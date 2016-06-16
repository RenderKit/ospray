#!/bin/bash

#node_counts=(1 2 3 5 9 17 33 65 129 257)
node_counts=(65 129)
queue="normal"
job_time="01:00:00"
job_name=rm_data_scale_min

# The script will run the volume run script from the specified work dir
export WORK_DIR=$WORK/ospray/build/rel/
export OUT_DIR=`pwd`
export VOLUME_RUN_SCRIPT=$WORK/ospray/rm_run.sh
#export VOLUME_RUN_SCRIPT=$WORK/ospray/dns_run.sh

export MIN_BRICK=1
export DATA_SCALING=1
export REPLICATED_DATA=0

for nodes in ${node_counts[@]}; do
	set -x
	sbatch -J ${job_name}$nodes -o ${job_name}${nodes}.txt -N $nodes -n $nodes \
		-p $queue -t $job_time $WORK/ospray/volume_viewer_bench.vnc -geometry 1600x1600
done

# Do the brick count increase runs with fixed number of nodes
#for bricks in ${brick_counts[@]}; do
#	set -x
  # TODO: Will slurm properly copy this environment variable per-submission? When does it copy
  # environment variables?
#  export RM_NUM_BRICKS=$bricks
#	sbatch -J ${job_name}$bricks -o ${job_name}${bricks}.txt -N ${node_counts[0]} -n ${node_counts[0]} \
#		-p $queue -t $job_time $WORK/ospray/volume_viewer_bench.vnc -geometry 1600x1600
#done

