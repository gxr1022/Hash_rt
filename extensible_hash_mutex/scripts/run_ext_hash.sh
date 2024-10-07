#!/bin/bash
# set -x
current=`date "+%Y-%m-%d-%H-%M-%S"`
RUN_PATH="/mnt/nvme0/home/gxr/hash_rt/extensible_hash_mutex/"
BINARY_PATH=${RUN_PATH}/build
LOG_PATH=${RUN_PATH}/ext_log/${current}
mkdir -p ${LOG_PATH}

pushd ${BINARY_PATH}

cmake -B ${BINARY_PATH} -DCMAKE_BUILD_TYPE=Release ${RUN_PATH}  2>&1 | tee ${RUN_PATH}/configure.log
if [[ "$?" != 0  ]];then
	exit
fi
cmake --build .

TEST_PATH=${BINARY_PATH}

num_of_ops_set=(1024  4096 8192)
time_interval=10
modes=(true false)

threads=(1)
for ((i = 4; i <= 32; i += 4)); do
    threads+=($i)
done

thread_binding_seq="0"
thread_bind=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 144 145 146 147 148 149 150 151 152 153 154)
for td in ${thread_bind[*]};do
    thread_binding_seq+=",$td"
done

test_name=(ext_client)

kv_sizes=(
	"8 100"
	# "8 1024"
	# "8 10240"
	# "8 102400"
	# "8 1048576"
)

for mode in "${modes[@]}"; do
for kv_size in "${kv_sizes[@]}";do
kv_size_array=( ${kv_size[*]} )
key_size=${kv_size_array[0]}
value_size=${kv_size_array[1]}

for t in ${threads[*]};do
for num_of_ops in ${num_of_ops_set[*]};do
for tn in ${test_name[*]};do
	
	cmd="${TEST_PATH}/${tn} \
	--num_threads=${t}  \
	--str_key_size=${key_size} \
	--str_value_size=${value_size} \
	--core_binding=${thread_binding_seq} \
	--time_interval=${time_interval} \
	--num_of_ops=${num_of_ops} \
	--first_mode=${mode}
	"
	this_log_path=${LOG_PATH}/${tn}.${t}.thread.${mode}.${key_size}.${value_size}.${num_of_ops}.ops.log
	echo ${cmd} 2>&1 | tee -a ${this_log_path}
	timeout -v 3600 stdbuf -o0 ${cmd} 2>&1 | tee -a ${this_log_path}
    echo "Log file in: ${this_log_path}"

	echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null

done
done 
done
done
done
popd

