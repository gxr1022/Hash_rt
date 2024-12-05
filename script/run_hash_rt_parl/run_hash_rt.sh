#!/bin/bash
# set -x
clean_build() {
    if [ -d "${BINARY_PATH}" ]; then
        rm -rf "${BINARY_PATH}"/*
    fi
}
current=`date "+%Y-%m-%d-%H-%M-%S"`
RUN_PATH="/mnt/nvme0/home/gxr/Myhash_boc/Hash_rt"
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

popd

pushd ${BINARY_PATH}

./hash_rt

# TEST_PATH=${BINARY_PATH}

# # num_of_ops_set=(1000 10000 100000 1000000)
# num_of_ops_set=(1000)
# modes=(true)
# work_usec=(0)
# batch_size=(32)
# # batch_size=(10 20 30 40 50 60 70 80 90 100) 
# kv_sizes=(
# 	# "8 100"
# 	# "8 1024"
# 	"8 100"
# 	# "8 102400"
# 	# "8 1048576"
# )

# threads_client=(8)
# # for ((i =9; i <= 32; i += 1)); do
# #     threads_client+=($i)
# # done

# threads_worker=(1)
# for ((i =2; i <= 16; i += 1)); do
#     threads_worker+=($i)
# done

# test_name=(hash_rt)
# for kv_size in "${kv_sizes[@]}";do
#     kv_size_array=( ${kv_size[*]} )
#     key_size=${kv_size_array[0]}
#     value_size=${kv_size_array[1]}
# for work in "${work_usec[@]}"; do
# for batch in "${batch_size[@]}"; do
# for mode in "${modes[@]}"; do
# for num_of_ops in ${num_of_ops_set[*]};do
#     ops_log_path=${LOG_PATH}/ops_${num_of_ops}
#     mkdir -p ${ops_log_path}
    
# for t_c in ${threads_client[*]};do
# for t_w in ${threads_worker[*]};do
# for tn in ${test_name[*]};do

# 	cmd="${TEST_PATH}/${tn} \
# 	--num_threads_client=${t_c} \
# 	--num_threads_worker=${t_w} \
# 	--str_key_size=${key_size} \
# 	--str_value_size=${value_size} \
# 	--num_ops=${num_of_ops} \
#     --first_mode=${mode} \
#     --work_usec=${work} \
#     --batch_size=${batch}
# 	"
# 	this_log_path=${ops_log_path}/${tn}.${t_c}.clientthread.${t_w}.workerthread.${mode}.${key_size}.${value_size}.${num_of_ops}.ops.${batch}.batch.log

# 	echo ${cmd} 2>&1 | tee -a ${this_log_path}
# 	# timeout -v 3600 stdbuf -o0 ${cmd} 2>&1 | tee -a ${this_log_path}
# 	${cmd} 2>&1 | tee -a ${this_log_path}
#     echo "Log file in: ${this_log_path}"
# 	echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
# done
# done 
# done
# done
# done
# done
# done
# done
# popd

# # Extract data and generate plot
# # python ${RUN_PATH}/script/run_hash_rt_parl/extract_hash_rt_time.py ${LOG_PATH}/ops_1000000
# # python ${RUN_PATH}/script/run_hash_rt_parl/plot_sched_run_avg_time.py ${LOG_PATH}/ops_1000000/time_metrics_sched_run.csv

# # echo "Plot saved to ${LOG_PATH}/ops_1000000/execution_time_plot.png"


