import os
import re
import csv
from collections import defaultdict

# Directory containing log files
directory = '/mnt/nvme0/home/gxr/hash_rt/hashmap_log/2024-10-08-00-21-32'

# Patterns to match filenames and extract data
file_pattern = re.compile(r'hash_rt\.(\d+)\.thread\.(true|false)\.(\d+)\.(\d+)\.(\d+)\.ops\.log')
time_pattern = re.compile(r'\[report\] \[report\] _overall_duration_ns\s*:\s*(\d+)')

# Results dictionary to store throughput for each combination of thread count and ops
results = defaultdict(lambda: defaultdict(dict))

# Read and process each file in the directory
for filename in os.listdir(directory):
    print(f'Processing: {filename}')
    match = file_pattern.match(filename)

    if match:
        # Extract thread count, true/false, and unique combination of ops
        thread_count = int(match.group(1))
        true_or_false = match.group(2)
        ops_combo = f"{match.group(3)}_{match.group(4)}_{match.group(5)}"  # Capture all components of ops combination
        
        # Only consider "true" Throughput
        if true_or_false == 'true':
            filepath = os.path.join(directory, filename)
            
            # Read file content and extract throughput
            with open(filepath, 'r') as file:
                content = file.read()
                time_match = time_pattern.search(content)
                if time_match:
                    throughput = time_match.group(1)
                    results[thread_count][ops_combo] = throughput

# Prepare output base directory
csv_base_dir = '../data'
folder_name = directory.split('/')[-1]
combined_dir = os.path.join(csv_base_dir, folder_name)
os.makedirs(combined_dir, exist_ok=True)

# Combine different ops into a single CSV file
combined_csv_filename = f'combined_true_latency.csv'
combined_csv_path = os.path.join(combined_dir, combined_csv_filename)

# Write the combined results to a single CSV
with open(combined_csv_path, 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    
    # Collect all ops combinations to create column headers
    all_ops = set()
    for thread_results in results.values():
        all_ops.update(thread_results.keys())
    all_ops = sorted(all_ops)
    
    # Write header row
    writer.writerow(['Thread Count'] + [f'Latency_{ops}' for ops in all_ops])
    
    # Sort thread counts and write data rows
    for thread_count in sorted(results.keys()):
        row = [thread_count] + [results[thread_count].get(ops, 'N/A') for ops in all_ops]
        writer.writerow(row)

print(f'Combined results have been saved to {combined_csv_path}')
