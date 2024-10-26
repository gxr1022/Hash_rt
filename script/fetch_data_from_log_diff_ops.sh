#!/bin/bash

RUN_PATH="/mnt/nvme0/home/gxr/hash_rt"
cur_date=$1

logs_folder="$RUN_PATH/hashmap_log/$cur_date"
output_csv="throughput_report.csv"

tmp_file=$(mktemp)

if [ ! -d "$logs_folder" ]; then
    echo "Log folder $logs_folder does not exist"
    exit 1
fi

declare -A throughput_map

for log_file in "$logs_folder"/*.log; do
    if [ ! -f "$log_file" ]; then
        echo "No valid log files found"
        exit 1
    fi

    thread_num=$(echo "$log_file" | awk -F'.' '{print $2}')
    ops_num=$(echo "$log_file" | awk -F'.' '{print $7}')
    
    throughput=$(grep "_overall_throughput" "$log_file" | awk -F': ' '{print $2}')
   
    if [ -z "$throughput" ]; then
        echo "No throughput data found in $log_file"
        continue
    fi
    throughput_map["$thread_num,$ops_num"]=$throughput
done

# header="thread_number,1e6_ops,1e7_ops,1e8_ops,1e9_ops"
header="thread_number,1e3_ops,1e4_ops,1e5_ops,1e6_ops,1e7_ops"
echo "$header" > "$output_csv"

# Extract unique thread numbers from the throughput_map keys and sort them numerically
thread_nums=$(for key in "${!throughput_map[@]}"; do echo "${key%,*}"; done | sort -n)

# Process each unique thread number
for thread_num in $thread_nums; do
    row="$thread_num"
    
    # Iterate over the predefined ops numbers (assuming these are ops: 1e6, 1e7, 1e8, 1e9)
    # for ops_num in 1000000 10000000 100000000 1000000000; do
    for ops_num in 1000 10000 100000 1000000 10000000; do
        # Retrieve the throughput value from the array, or leave empty if not found
        value="${throughput_map["$thread_num,$ops_num"]}"
        row="$row,${value:-}"
    done
    
    # Write the complete row to the CSV file (each row corresponds to one thread_num)
    echo "$row" >> "$output_csv"
done

# Clean up the temporary file
rm "$tmp_file"

echo "CSV file successfully generated: $output_csv"
