#!/bin/bash

#node_counts=(2 3 5 9 17 33 65 129)
node_counts=(9 17 33 65)
queue="normal"
job_time="00:35:00"
job_name=rm_data_scaling

# The script will run the volume run script from the specified work dir
export WORK_DIR=$WORK/ospray/build/rel/
export OUT_DIR=`pwd`
export VOLUME_RUN_SCRIPT=$WORK/ospray/rm_run.sh

for nodes in ${node_counts[@]}; do
	set -x
	sbatch -J ${job_name}$nodes -o ${job_name}${nodes}.txt -N $nodes -n $nodes \
		-p $queue -t $job_time $WORK/ospray/volume_viewer_bench.vnc -geometry 1600x1600
done

