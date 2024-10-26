import pandas as pd
# import matplotlib.pyplot as plt

def merge_throughput_files(file1, file2, file3,output_file):
    # Load the data from each file
    boc_df = pd.read_csv(file1, usecols=["Thread Count", "true Throughput"])
    mutex_df = pd.read_csv(file2, usecols=["Thread Count", "true Throughput"])
    libcuckoo_df = pd.read_csv(file3, usecols=["Thread Count", "true Throughput"])

    # Rename the true Throughput columns for clarity
    boc_df.rename(columns={"true Throughput": "boc True Throughput"}, inplace=True)
    mutex_df.rename(columns={"true Throughput": "mutex True Throughput"}, inplace=True)
    libcuckoo_df.rename(columns={"true Throughput": "libcuckoo True Throughput"}, inplace=True)

    # Merge the three dataframes on Thread Count
    merged_df = boc_df.merge(mutex_df, on="Thread Count").merge(libcuckoo_df, on="Thread Count")
    merged_df.to_csv(output_file, index=False)
    # return merged_df

# file1="/mnt/nvme0/home/gxr/hash_rt/data/2024-10-11-23-38-49/8_100_mutex_overall_throughput.csv"
# file2="/mnt/nvme0/home/gxr/hash_rt/extensible_hash_mutex/data/2024-10-11-23-33-40/8_100_mutex_overall_throughput.csv"
# file3="/mnt/nvme0/home/gxr/libcuckoo/data/2024-10-11-23-03-05/8_100_mutex_overall_throughput.csv"

# file1="/mnt/nvme0/home/gxr/hash_rt/data/2024-10-11-23-59-26/8_1024_mutex_overall_throughput.csv"
# file2="/mnt/nvme0/home/gxr/hash_rt/extensible_hash_mutex/data/2024-10-12-00-08-25/8_1024_mutex_overall_throughput.csv"
# file3="/mnt/nvme0/home/gxr/libcuckoo/data/2024-10-12-00-06-40/8_1024_mutex_overall_throughput.csv"

file1="/mnt/nvme0/home/gxr/hash_rt/data/2024-10-11-23-59-26/8_102400_mutex_overall_throughput.csv"
file2="/mnt/nvme0/home/gxr/hash_rt/extensible_hash_mutex/data/2024-10-12-00-08-25/8_102400_mutex_overall_throughput.csv"
file3="/mnt/nvme0/home/gxr/libcuckoo/data/2024-10-12-00-06-40/8_102400_mutex_overall_throughput.csv"


merge_throughput_files(file1, file2, file3,'merged_8_102400.csv')
# print(merged_df)
# plot_throughput_vs_thread_count(merged_df)

