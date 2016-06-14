#!/bin/bash

#node_counts=(2 3 5 9 17 33 65 129)
node_counts=(129)
brick_counts=(3 5 9 17 33 65 129)

queue="normal"
job_time="00:10:00"
job_name=rm_brick_increase

# The script will run the volume run script from the specified work dir
export WORK_DIR=$WORK/ospray/build/rel/
export OUT_DIR=`pwd`
export VOLUME_RUN_SCRIPT=$WORK/ospray/rm_run.sh

#for nodes in ${node_counts[@]}; do
#	set -x
#	sbatch -J ${job_name}$nodes -o ${job_name}${nodes}.txt -N $nodes -n $nodes \
#		-p $queue -t $job_time $WORK/ospray/volume_viewer_bench.vnc -geometry 1600x1600
#done

# Do the brick count increase runs with fixed number of nodes
for bricks in ${brick_counts[@]}; do
	set -x
  # TODO: Will slurm properly copy this environment variable per-submission? When does it copy
  # environment variables?
  export NUM_BRICKS=$bricks
	sbatch -J ${job_name}$bricks -o ${job_name}${bricks}.txt -N ${node_counts[0]} -n ${node_counts[0]} \
		-p $queue -t $job_time $WORK/ospray/volume_viewer_bench.vnc -geometry 1600x1600
done

