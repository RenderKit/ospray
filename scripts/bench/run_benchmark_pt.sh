#!/bin/sh

if [ $# -lt 2 ]; then
  echo "No dimensions passed, using defaults"
  bench_img_width=1024
  bench_img_height=1024
else
  bench_img_width=$1
  bench_img_height=$2
fi

echo "All tests run at $bench_img_width x $bench_img_height"

exe="./ospBenchmark -w $bench_img_width -h $bench_img_height"

img_dir="test_images"

# Create image directory
if [ ! -d ${img_dir} ]; then
  mkdir ${img_dir}
fi

runBenchmark()
{
  name=$1
  file=$2
  camera="$3"
  params="$4"
  run_name=${name}_scivis
  echo -n "test[$run_name] : "
  ${exe} ${file} ${camera} -bf 100 -wf 50 -r sv --variance -spp 1 --ao-distance 1e8f --sun-int 0 --ao-samples 1 --noshadows --bg-color 1 1 1 -i ${img_dir}/test_${run_name} ${params}
  run_name=${name}_pt
  echo -n "test[$run_name] : "
  ${exe} ${file} ${camera} -bf 100 -wf 50 -r pt --variance --max-depth 1 --sun-int 0 -i ${img_dir}/test_${run_name} ${params}
}

# Test counter
test_num=0

# FIU TESTS #################################################################

camera="-vp 500.804565 277.327850 -529.199829 \
    -vu 0.000000 1.000000 0.000000 \
    -vi 21.162066 -62.059830 -559.833313"
runBenchmark "fiu1" "test_data/fiu-groundwater.xml" "$camera"
camera="-vp -29.490566 80.756294 -526.728516 \
    -vu 0.000000 1.000000 0.000000 \
    -vi 21.111689 12.973234 -443.164886"
runBenchmark "fiu2" "test_data/fiu-groundwater.xml" "$camera"

# LLNL TESTS #################################################################

#test_num=$((test_num+1))
camera="-vp 3371.659912 210.557999 -443.156006 \
    -vu -0.000000 -0.000000 -1.000000 \
    -vi 1439.359985 1005.450012 871.119019"
runBenchmark "llnl1" "test_data/llnl-2048-iso.xml" "$camera"

camera="-vp 2056.597168 999.748108 402.587219 \
    -vu -0.000000 -0.000000 -1.000000 \
    -vi 1439.358887 1005.449951 871.118164"
runBenchmark "llnl2" "test_data/llnl-2048-iso.xml" "$camera"

# SPONZA TESTS #################################################################

camera="-vp 667.492554 186.974228 76.008301 \
-vu 0.000000 1.000000 0.000000 -vi 84.557503 188.199417 -38.148270"
params="--ambient 1 1 1 12"
runBenchmark "sponza" "test_data/crytek-sponza/sponza.obj" "$camera" "$params"

# SAN-MIGUEL TESTS #################################################################

camera="-vp -2.198506 3.497189 23.826025 -vu 0.000000 1.000000 0.000000\
-vi -2.241950 2.781175 21.689358"
params="--ambient 1 1 1 12"
runBenchmark "sanmiguel" "test_data/san-miguel/sanMiguel.obj" "$camera" "$params"

# SIBENIK TESTS #################################################################

camera="-vp -17.734447 -13.788272 3.443677 -vu 0.000000 1.000000 0.000000\
-vi -2.789550 -10.993323 0.331822"
params="--ambient 1 1 1 12"
runBenchmark "sibenik" "test_data/sibenik/sibenik.obj" "$camera" "$params"
