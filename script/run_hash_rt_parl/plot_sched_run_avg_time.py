import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

csv_path = sys.argv[1]
output_dir = os.path.dirname(csv_path)
output_filename = os.path.join(output_dir, 'execution_time_plot.png')

df = pd.read_csv(csv_path)

fig, ax1 = plt.subplots(figsize=(12, 6))

ax2 = ax1.twinx()

line1 = ax1.plot(range(1, 21), df['scheduler_avg_when_time_ns'], 
                 color='blue', marker='o', label='Scheduler Average Time')
line2 = ax2.plot(range(1, 21), df['insert_avg_insert_time_ns'], 
                 color='red', marker='s', label='Insert Average Time')

ax1.set_xlabel('Hardware Threads', color='black')
ax1.set_ylabel('Scheduler Time (ns)', color='black')
ax2.set_ylabel('Insert Time (ns)', color='black')

ax1.set_xticks(range(1, 21))

lines = line1 + line2
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper left')

ax1.grid(True, linestyle='--', alpha=0.7)

plt.title('Thread Count vs Average Execution Time')
plt.savefig(output_filename)