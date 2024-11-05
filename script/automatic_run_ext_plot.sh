/users/Xuran/hash_rt/script/run_hash_rt.sh

latest_folder=$(ls -t ../log | grep -E '^[0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{2}-[0-9]{2}-[0-9]{2}$' | head -n1)

/users/Xuran/hash_rt/script/fetch_data_from_log_fixed_ops.sh "$latest_folder"
python /users/Xuran/hash_rt/script/plot_hash_rt_throughput.py  "$latest_folder"
