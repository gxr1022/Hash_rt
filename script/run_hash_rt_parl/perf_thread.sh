ps aux | grep hash_rt
hash_rt_pid=$(pgrep hash_rt)

sudo perf record -p $hash_rt_pid -T -e sched:sched_switch,cpu-clock -g --call-graph dwarf -o hash_perf.data -- sleep 30

sudo perf report -i hash_perf.data --sort cpu,comm,pid

sudo perf sched map --input hash_perf.data

sudo perf sched timehist --pid $hash_rt_pid --input hash_perf.data > cpu_usage.txt

cat cpu_usage.txt
