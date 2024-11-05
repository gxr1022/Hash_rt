#!/bin/bash

RUN_PATH="/users/Xuran/hash_rt"
cur_date=$1

logs_folder="$RUN_PATH/log/$cur_date"
output_csv="$RUN_PATH/data/$cur_date/throughput_report.csv"

mkdir -p "$RUN_PATH/data/$cur_date"

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
    
    
    throughput=$(grep "insert_overall_throughput" "$log_file" | awk -F': ' '{print $3}')
   
    if [ -z "$throughput" ]; then
        echo "No throughput data found in $log_file"
        continue
    fi
    throughput_map["$thread_num,$ops_num"]=$throughput
done

thread_nums=$(for key in "${!throughput_map[@]}"; do
    echo "$key" | cut -d',' -f1
done | sort -n | uniq)

header="thread_number,1e6_ops"
echo "$header" > "$output_csv"

for thread_num in $thread_nums; do
    row="$thread_num"
    
    ops_num=1000000
    value="${throughput_map["$thread_num,$ops_num"]}"
    row="$row,${value:-}"
    
    echo "$row" >> "$output_csv"
done

header="thread_number,1e6_ops"
echo "$header" > "$output_csv"

for thread_num in $thread_nums; do
    row="$thread_num"
    
    ops_num=1000000
    value="${throughput_map["$thread_num,$ops_num"]}"
    row="$row,${value:-}"
    
    echo "$row" >> "$output_csv"
done

# Clean up the temporary file
rm "$tmp_file"

echo "CSV file successfully generated: $output_csv"
