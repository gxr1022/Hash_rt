import os
import re
import csv
from collections import defaultdict


# directory = '/mnt/nvme0/home/gxr/hash_rt/extensible_hash_mutex/ext_log/2024-10-01-06-08-29'
directory = '/mnt/nvme0/home/gxr/hash_rt/hashmap_log/2024-10-24-00-01-15'

# file_pattern = re.compile(r'ext_client\.(\d+)\.thread\.(true|false)\.(\d+)\.(\d+)\.(\d+s)\.log')
# file_pattern = re.compile(r'ext_client\.(\d+)\.thread\.(true|false)\.(\d+)\.(\d+)\.(\d+)\.s\.log')
# file_pattern = re.compile(r'hash_rt\.(\d+)\.thread\.(true|false)\.(\d+)\.(\d+)\.1000000\.ops\.log')
file_pattern = re.compile(r'hash_rt\.(\d+)\.thread\.(true|false)\.(\d+)\.(100)\.1000000\.ops\.log')

time_pattern = re.compile(r'\[report\] \[report\] _overall_throughput\s*:\s*([\d\.]+)')
# time_pattern = re.compile(r'\[report\] \[report\] _overall_duration_ns\s*:\s*(\d+)')
results = defaultdict(lambda: defaultdict(dict))

for filename in os.listdir(directory):
    print(f'Processing: {filename}')
    match = file_pattern.match(filename)

    if match:
        thread_count = int(match.group(1))
        true_or_false = match.group(2)
        combo = f"{match.group(3)}_{match.group(4)}"  # e.g., "8_100" or "8_1024"
        
        filepath = os.path.join(directory, filename)
        
        with open(filepath, 'r') as file:
            content = file.read()
            time_match = time_pattern.search(content)
            if time_match:
                throughput = time_match.group(1)
                results[combo][thread_count][true_or_false] = throughput

csv_base_dir = '/mnt/nvme0/home/gxr/hash_rt/data'
folder_name = directory.split('/')[-1]
combined_dir = os.path.join(csv_base_dir, folder_name)
os.makedirs(combined_dir, exist_ok=True)


for combo, thread_results in results.items():
    csv_filename = f'{combo}_hash_rt_overall_throughput.csv'
    combined_path = os.path.join(combined_dir, csv_filename)

    with open(combined_path, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Thread Count', 'true Throughput', 'false Throughput'])
        sorted_thread_results = sorted(thread_results.items())
        
        for thread_count, throughputs in sorted_thread_results:
            true_value = throughputs.get('true', 'N/A')
            false_value = throughputs.get('false', 'N/A')
            print(true_value, false_value)
            writer.writerow([thread_count, true_value, false_value])

    print(f'Results for {combo} have been saved to {combined_path}')
