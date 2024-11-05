#!/bin/bash
chmod +x ../build/hash_rt

perf record  -g --call-graph=dwarf ../build/hash_rt --num_ops=1000000
if [ $? -ne 0 ]; then
    echo "Error: perf record failed"
    exit 1
fi

if [ ! -s perf.data ]; then
    echo "Error: perf.data is empty or does not exist"
    exit 1
fi

perf report > perf_report.txt
perf script > perf_script.txt
perf script > out.perf

if [ ! -d "FlameGraph" ]; then
    echo "Cloning FlameGraph repository..."
    git clone https://github.com/brendangregg/FlameGraph.git
fi

./FlameGraph/stackcollapse-perf.pl out.perf | ./FlameGraph/flamegraph.pl > flamegraph.svg