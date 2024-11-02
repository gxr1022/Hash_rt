perf record -g --call-graph=dwarf ../build/hash_rt
perf report > perf_report.txt
perf script > perf_script.txt
perf script > out.perf
if [ ! -d "FlameGraph" ]; then
  echo "Cloning FlameGraph repository..."
  git clone https://github.com/brendangregg/FlameGraph.git
fi
./FlameGraph/stackcollapse-perf.pl out.perf | ./FlameGraph/flamegraph.pl > flamegraph.svg