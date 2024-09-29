#!/bin/bash
# set -x
current=`date "+%Y-%m-%d-%H-%M-%S"`
RUN_PATH="/mnt/nvme0/home/gxr/hash_rt"
BINARY_PATH=${RUN_PATH}/build
LOG_PATH=${RUN_PATH}/hashmap_log/${current}
mkdir -p ${LOG_PATH}

pushd ${BINARY_PATH}

cmake -B ${BINARY_PATH} -GNinja -DCMAKE_BUILD_TYPE=Debug ${RUN_PATH}  2>&1 | tee ${RUN_PATH}/configure.log
if [[ "$?" != 0  ]];then
	exit
fi
cmake --build .

TEST_PATH=${BINARY_PATH}


threads=(1)
for ((i = 4; i <= 64; i += 4)); do
    threads+=($i)
done


num_of_ops_set=( 100000)


# test_name=(func-sys-hashmap func-con-hashmap)
test_name=(hash_rt)

for t in ${threads[*]};do
for num_of_ops in ${num_of_ops_set[*]};do
for tn in ${test_name[*]};do
	
	cmd="${TEST_PATH}/${tn} \
	--cores ${t} \
	--number_of_ops ${num_of_ops}
	"
	this_log_path=${LOG_PATH}/${tn}.${t}.thread.${num_of_ops}.ops.log
	echo ${cmd} 2>&1 | tee -a ${this_log_path}
	timeout -v 3600 stdbuf -o0 ${cmd} 2>&1 | tee -a ${this_log_path}
    echo "Log file in: ${this_log_path}"
done
done 
done
popd

