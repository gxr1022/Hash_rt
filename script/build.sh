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


