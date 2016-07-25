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

# Test counter
test_num=0

# FIU TESTS #################################################################

fiu="test_data/fiu-groundwater.xml"

echo -n "test[$test_num] : "

# FIU/scivis/view 1
${exe} ${fiu} \
    -vp 500.804565 277.327850 -529.199829 \
    -vu 0.000000 1.000000 0.000000 \
    -vi 21.162066 -62.059830 -559.833313 \
    -r scivis \
    -i ${img_dir}/test_${test_num}

echo " : FIU/scivis/view 1"

test_num=$((test_num+1))

echo -n "test[$test_num] : "

# FIU/scivis/view 2
${exe} ${fiu} \
    -vp -29.490566 80.756294 -526.728516 \
    -vu 0.000000 1.000000 0.000000 \
    -vi 21.111689 12.973234 -443.164886 \
    -r scivis \
    -i ${img_dir}/test_${test_num}

echo " : FIU/scivis/view 2"

test_num=$((test_num+1))

# HEPTANE TESTS ##############################################################

csafe="test_data/csafe-heptane-302-volume.osp"

echo -n "test[$test_num] : "

# CSAFE/scivis/view 1
${exe} ${csafe} \
    -r scivis \
    -vp 286.899994 422.800018 -30.200012 \
    -vi 151.000000 151.000000 151.000000
    -vu 0 1 0 \
    -tfc 0 0 0 0.00 \
    -tfc 1 0 0 0.11 \
    -tfc 1 1 0 0.22 \
    -tfc 1 1 1 0.33 \
    -tfs 0.25 \
    -i ${img_dir}/test_${test_num}

echo " : CSAFE/scivis/view 1"

test_num=$((test_num+1))

echo -n "test[$test_num] : "

# CSAFE/scivis/view 2
${exe} ${csafe} \
    -r scivis \
    -vp -36.2362 86.8541 230.026 \
    -vi 150.5 150.5 150.5 \
    -vu 0 0 1 \
    -tfc 0 0 0 0.00 \
    -tfc 1 0 0 0.11 \
    -tfc 1 1 0 0.22 \
    -tfc 1 1 1 0.33 \
    -tfs 0.25 \
    -i ${img_dir}/test_${test_num}

echo " : CSAFE/scivis/view 2"

test_num=$((test_num+1))

# MAGNETIC TESTS #############################################################

magnetic="test_data/magnetic-512-volume.osp"

echo -n "test[$test_num] : "

# MAGNETIC/scivis/view 1
${exe} ${magnetic} \
    -r scivis \
    -vp 255.5 -1072.12 255.5 \
    -vi 255.5 255.5 255.5 \
    -vu 0 0 1 \
    -tfc 0.1 0.1 0.1 0 \
    -tfc 0 0 1 0.25 \
    -tfc 1 0 0 1 \
    -tfs 1 \
    -i ${img_dir}/test_${test_num}

echo " : MAGNETIC/scivis/view 1"

test_num=$((test_num+1))

echo -n "test[$test_num] : "

# MAGNETIC/scivis/view 2
${exe} ${magnetic} \
    -r scivis \
    -vp 431.923 -99.5843 408.068 \
    -vi 255.5 255.5 255.5 \
    -vu 0 0 1 \
    -tfc 0.1 0.1 0.1 0 \
    -tfc 0 0 1 0.25 \
    -tfc 1 0 0 1 \
    -tfs 1 \
    -i ${img_dir}/test_${test_num}

echo " : MAGNETIC/scivis/view 2"

test_num=$((test_num+1))

echo -n "test[$test_num] : "

# MAGNETIC/scivis/view 2 + isosurface
${exe} ${magnetic} \
    -r scivis \
    -vp 431.923 -99.5843 408.068 \
    -vi 255.5 255.5 255.5 \
    -vu 0 0 1 \
    -tfc 0.1 0.1 0.1 0 \
    -tfc 0 0 1 0.25 \
    -tfc 1 0 0 1 \
    -tfs 1 \
    -is 2.0 \
    -i ${img_dir}/test_${test_num}

echo " : MAGNETIC/scivis/view 2 + isosurface"

test_num=$((test_num+1))

# LLNL ISO TESTS  ############################################################

llnl_iso="test_data/llnl-2048-iso.xml"

echo -n "test[$test_num] : "

# LLNL_ISO/scivis/view 1
${exe} ${llnl_iso} \
    -r scivis \
    -vp 3371.659912 210.557999 -443.156006 \
    -vu -0.000000 -0.000000 -1.000000 \
    -vi 1439.359985 1005.450012 871.119019 \
    -i ${img_dir}/test_${test_num}

echo " : LLNL-ISO/scivis/view 1"

test_num=$((test_num+1))

echo -n "test[$test_num] : "

# LLNL_ISO/scivis/view 2
${exe} ${llnl_iso} \
    -r scivis \
    -vp 2056.597168 999.748108 402.587219 \
    -vu -0.000000 -0.000000 -1.000000 \
    -vi 1439.358887 1005.449951 871.118164 \
    -i ${img_dir}/test_${test_num}

echo " : LLNL-ISO/scivis/view 2"

test_num=$((test_num+1))
