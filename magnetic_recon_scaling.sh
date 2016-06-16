#!/bin/bash

# Queue up a ton of runs to do scaling tests on the Magnetic Reconnection at various node counts
# Note that for distributed runs we have one extra node for the master though the 2 node
# case is a test of 1 worker 1 master to compare with a single combined worker/master when
# running non-distributed with a single node
#node_counts=(1 2 3 5 9 17 33 65 129)
node_counts=(1 2)
# Use these node counts for the spp scaling
#node_counts=(1 5 17 65 129)
queue="normal"
job_time="01:30:00"
job_name=magnetic_recon_strong

export WORK_DIR=$WORK/ospray/build/rel/
export OUT_DIR=`pwd`
export RUN_SCRIPT=$WORK/ospray/magnetic_recon_run.sh

for nodes in ${node_counts[@]}; do
	set -x
	sbatch -J ${job_name}${nodes} -o ${job_name}${nodes}.txt -N $nodes -n $nodes \
		-p $queue -t $job_time $RUN_SCRIPT
done

