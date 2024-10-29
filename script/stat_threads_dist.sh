#!/bin/bash

cur_date=$1
RUN_PATH="/mnt/nvme0/home/gxr/hash_rt"
LOG_DIR="/mnt/nvme0/home/gxr/hash_rt/hashmap_log/$cur_date"
STAT_DIR="$RUN_PATH/stats/$cur_date"

mkdir -p $STAT_DIR

if [ ! -d "$LOG_DIR" ]; then
  echo "Log directory $LOG_DIR does not exist."
  exit 1
fi

for log_file in "$LOG_DIR"/*.log; do
  base_name=$(basename "$log_file" .log)
  
  unset thread_counts
  declare -A thread_counts
  total_count=0

  while read -r line; do
    if [[ "$line" =~ [[:space:]]*Set\ running\ thread:\ *([0-9]+) ]]; then
        thread_id="${BASH_REMATCH[1]}"
        ((thread_counts["$thread_id"]++))
        ((total_count++)) 
    fi
  done < "$log_file"

  output_file="$STAT_DIR/${base_name}_thread_count_summary.txt"
  echo "Thread ID counts from $log_file:" > "$output_file"
  
  for thread_id in "${!thread_counts[@]}"; do
    echo "Thread $thread_id: ${thread_counts[$thread_id]} times" >> "$output_file"
  done

  echo "" >> "$output_file"
  echo "Total 'Set running thread' calls: $total_count" >> "$output_file"
  
  echo "Summary saved to $output_file"
done
