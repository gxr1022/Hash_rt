#!/bin/bash

base_dir="/users/Xuran/hash_rt/log"
date_folder=$1
work_dir="$base_dir/$date_folder"
# echo $work_dir

if [ ! -d "$work_dir" ]; then
    echo "directory: $work_dir does not exist"
    exit 1
fi

if [ -z "$(ls -A $work_dir)" ]; then
    echo "directory: $work_dir is empty"
    exit 1
fi

for sub_dir in "$work_dir"/*; do
    echo $sub_dir
    if [ -d "$sub_dir" ]; then
        output_file="$sub_dir/detailed_time_analysis.csv"
        echo "cores,init_hash_tabletime,init_scheduler_time,schedule_behaviours_time,run_behaviours_time" > "$output_file"

        for logfile in "$sub_dir"/*.log; do

            # echo $logfile
            if [ -f "$logfile" ]; then
                cores=$(basename "$logfile" | sed 's/.*\.\([0-9]\+\)\.thread\..*/\1/')
                init_time=$(grep "Init time:" "$logfile" | awk -F: '{print $2}' | tr -d ' ')
                init_scheduler_time=$(grep "Init scheduler time:" "$logfile" | awk -F: '{print $2}' | tr -d ' ')
                schedule_behaviours_time=$(grep "Schedule behaviours time:" "$logfile" | awk -F: '{print $2}' | tr -d ' ')
                run_behaviours_time=$(grep "Run behaviours time:" "$logfile" | awk -F: '{print $2}' | tr -d ' ')
                echo "$cores,$init_time,$init_scheduler_time,$schedule_behaviours_time,$run_behaviours_time" >> "$output_file"
            fi
        done
        sort -t',' -k1,1n -o "$output_file" "$output_file"
    fi
done

