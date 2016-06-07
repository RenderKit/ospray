#!/bin/bash

#node_counts=(1 2 3 5 9 17 33 65)
node_counts=(2 3)
queue="normal"
job_time="00:10:00"
job_name=rm_min_brick_tile32_

for nodes in ${node_counts[@]}; do
	set -x
	sbatch -J ${job_name}$nodes -o ${job_name}${nodes}.txt -N $nodes -n $nodes \
		-p $queue -t $job_time ../../volume_viewer_bench.vnc -geometry 1600x1600
done

