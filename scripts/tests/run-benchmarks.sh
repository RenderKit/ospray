#!/bin/bash -xe

## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

git log -1

# environment for benchmark client
source ~/benchmark_client.git/env.sh
source ~/system_token.sh

# benchmark configuration
SOURCE_ROOT=`pwd`
PROJECT_NAME="OSPRay"
BENCHMARK_FLAGS="--benchmark_min_time=${BENCHMARK_MIN_TIME_SECONDS:-10}"

export LD_LIBRARY_PATH=`pwd`/build/install/ospray/lib:${LD_LIBRARY_PATH}

cd build/install/ospray/bin
rm -f *.json

################################# PLEASE READ ##################################
#
# Note that suites and subsuites must exist in the database _before_ attempting
# insertion of results. This is intentional! You should think carefully about
# your [suite -> subsuite -> benchmark] hierarchy and definitions. These should
# be stable over time (especially for suites and subsuites) to facilitate
# long-term comparisons.
#
# These can be inserted using the benchmark client, through the "insert suite"
# and "insert subsuite" commands. Ask for help if you have questions.
#
################################# PLEASE READ ###################################

######################
# Example benchmarks #
######################

SUITE_NAME="Examples"

SUBSUITE_NAME="BoxesAO"
SUBSUITE_REGEX="^boxes"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json

# wait to insert contexts (which will be reused for all subsequent benchmark runs)
# until first benchmark successfully finishes.
benny insert code_context "${PROJECT_NAME}" ${SOURCE_ROOT} --save-json code_context.json
benny insert run_context ${TOKEN} ./code_context.json --save-json run_context.json

benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

# subsequent subsuite runs...

SUBSUITE_NAME="CornellBoxSPP"
SUBSUITE_REGEX="^cornell_box"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="GravitySpheresVolumeDIM"
SUBSUITE_REGEX="^gravity_spheres_volume"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="PerlinNoiseVolumes"
SUBSUITE_REGEX="^perlin_noise_volumes"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="ParticleVolumes"
SUBSUITE_REGEX="^particle_volume"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="UnstructuredVolumes"
SUBSUITE_REGEX="^unstructured_volume"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="Clipping"
SUBSUITE_REGEX="^clip"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

SUBSUITE_NAME="Other"
SUBSUITE_REGEX="^(random_spheres|streamlines|planes|vdb_volume|gravity_spheres_amr)"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json

###################
# Microbenchmarks #
###################

SUITE_NAME="Microbenchmarks"

SUBSUITE_NAME="Setup"
SUBSUITE_REGEX="^(ospInit_ospShutdown|setup)"
./ospBenchmark ${BENCHMARK_FLAGS} --benchmark_filter=${SUBSUITE_REGEX} --benchmark_out=results-${SUITE_NAME}-${SUBSUITE_NAME}.json
benny insert googlebenchmark ./run_context.json ${SUITE_NAME} ${SUBSUITE_NAME} ./results-${SUITE_NAME}-${SUBSUITE_NAME}.json
