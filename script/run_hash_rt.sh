#!/bin/bash
# set -x
clean_build() {
    if [ -d "${BINARY_PATH}" ]; then
        rm -rf "${BINARY_PATH}"/*
    fi
}
current=`date "+%Y-%m-%d-%H-%M-%S"`
RUN_PATH="/users/Xuran/hash_rt"
BINARY_PATH=${RUN_PATH}/build
LOG_PATH=${RUN_PATH}/log/${current}
mkdir -p ${LOG_PATH}

pushd ${BINARY_PATH}

clean_build

cmake -B ${BINARY_PATH} -GNinja -DUSE_SYSTEMATIC_TESTING=OFF -DCMAKE_BUILD_TYPE=Debug ${RUN_PATH}  2>&1 | tee ${RUN_PATH}/configure.log
if [[ "$?" != 0  ]];then
	exit
fi
cmake --build . 

TEST_PATH=${BINARY_PATH}

# num_of_ops_set=(1000 10000 100000 1000000)
num_of_ops_set=(1000000)
modes=(true)
work_usec=(0)
kv_sizes=(
	# "8 100"
	# "8 1024"
	"8 100"
	# "8 102400"
	# "8 1048576"
)

threads=(4)
for ((i =6; i <= 40; i += 2)); do
    threads+=($i)
done

test_name=(hash_rt)

for kv_size in "${kv_sizes[@]}";do
    kv_size_array=( ${kv_size[*]} )
    key_size=${kv_size_array[0]}
    value_size=${kv_size_array[1]}
for work in "${work_usec[@]}"; do
for mode in "${modes[@]}"; do
for num_of_ops in ${num_of_ops_set[*]};do
    ops_log_path=${LOG_PATH}/ops_${num_of_ops}
    mkdir -p ${ops_log_path}
    
for t in ${threads[*]};do
for tn in ${test_name[*]};do

	cmd="${TEST_PATH}/${tn} \
	--num_threads=${t} \
	--str_key_size=${key_size} \
	--str_value_size=${value_size} \
	--num_ops=${num_of_ops} \
    --first_mode=${mode} \
    --work_usec=${work}
	"
	this_log_path=${ops_log_path}/${tn}.${t}.thread.${mode}.${key_size}.${value_size}.${num_of_ops}.ops.log

	echo ${cmd} 2>&1 | tee -a ${this_log_path}
	timeout -v 3600 stdbuf -o0 ${cmd} 2>&1 | tee -a ${this_log_path}
    echo "Log file in: ${this_log_path}"
	echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
done
done 
done
done
done
done
popd

# # ${RUN_PATH}/script/extract_partitioned_time.sh ${current}
# # python ${RUN_PATH}/script/plot_combined_bar_diff_cores.py ${current}