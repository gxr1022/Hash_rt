import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys

def plot_throughput(date_str):
    plt.rcParams['font.sans-serif'] = ['SimHei']
    plt.rcParams['axes.unicode_minus'] = False

    file_path = os.path.join('..', 'data', date_str, 'throughput_report.csv')
    
    if not os.path.exists(file_path):
        print(f"Error: File not found {file_path}")
        sys.exit(1)

    df = pd.read_csv(file_path)

    plt.figure(figsize=(12, 8))
    sns.set_style("whitegrid")

    plt.plot(df['thread_number'], df['1e6_ops'], marker='o', linewidth=2, markersize=6)

    plt.title('Thread Count vs Throughput', fontsize=14)
    plt.xlabel('Number of Threads', fontsize=12)
    plt.ylabel('Throughput (ops)', fontsize=12)

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()

    output_path = os.path.join('..', 'data', date_str, 'throughput_analysis.png')
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Image saved to: {output_path}")

    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python plot_hash_rt_throughput.py <date>")
        print("Example: python plot_hash_rt_throughput.py 2024-11-04-03-58-55")
        sys.exit(1)
    
    date_str = sys.argv[1]
    plot_throughput(date_str)
