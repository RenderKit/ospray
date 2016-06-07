#!/bin/bash

# Queue up a ton of runs to do scaling tests on the Magnetic Reconnection at various node counts
# Note that for distributed runs we have one extra node for the master though the 2 node
# case is a test of 1 worker 1 master to compare with a single combined worker/master when
# running non-distributed with a single node
#node_counts=(1 2 3 5 9 17 33 65)
# Use these node counts for the spp scaling
node_counts=(1 5 17 65 129)
queue="normal"
job_time="00:15:00"

for nodes in ${node_counts[@]}; do
	set -x
	sbatch -J magnetic_recon_bench_spp$nodes -o magnetic_recon_bench_spp${nodes}.txt -N $nodes -n $nodes \
		-p $queue -t $job_time ../../magnetic_recon_run.sh
done

