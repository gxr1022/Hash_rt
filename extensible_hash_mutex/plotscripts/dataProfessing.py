import os
import re
import csv


# directory = '/mnt/nvme0/home/gxr/hash_rt/extensible_hash_mutex/ext_log/2024-09-27-06-14-10'  
# directory = '/mnt/nvme0/home/gxr/hash_rt/hashmap_log/2024-09-27-06-37-50'
directory = '/mnt/nvme0/home/gxr/Myhash/log/2024-09-27-07-39-15'

# file_pattern = re.compile(r'ext_client\.(\d+)\.thread\.1000000\.ops')
# file_pattern = re.compile(r'hash_rt\.(\d+)\.thread\.10000000\.ops\.log')


file_pattern = re.compile(r'myhash\.(\d+)\.thread\.16\.4096\.1000000\.ops\.log')
# time_pattern = re.compile(r'Execution Time:\s*([\d\.e\+]+)\s*s')

time_pattern = re.compile(r'\[report\] load_overall_duration_s\s*:\s*([\d\.]+)')

results = []

for filename in os.listdir(directory):
    print(filename)
    match = file_pattern.match(filename)
    if match:
        thread_count = int(match.group(1))  
        filepath = os.path.join(directory, filename)
        

        with open(filepath, 'r') as file:
            content = file.read()
            print(content)

            time_match = time_pattern.search(content)
            if time_match:
                execution_time = time_match.group(1)
                results.append((thread_count, execution_time))


results.sort(key=lambda x: x[0])
print(results)

# csv_filename = 'mutex_execution_times.csv'
csv_filename = 'free_execution_times_4096.csv'
with open(csv_filename, 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(['Thread Count', 'Execution Time (s)'])
    writer.writerows(results)

print(f'Results have been saved to {csv_filename}')
