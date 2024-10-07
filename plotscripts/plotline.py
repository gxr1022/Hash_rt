import matplotlib.pyplot as plt

# Data from the CSV format
thread_counts = [1, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64]
true_throughput = [5376.0, 7633.0, 8130.0, 7692.0, 7575.0, 7812.0, 7299.0, 7692.0, 7352.0, 6622.0, 5780.0, 5917.0, 5952.0, 5813.0, 6172.0, 5952.0, 6711.0]
false_throughput = [5376.0, 7936.0, 9708.0, 8771.0, 8547.0, 10526.0, 13888.0, 14492.0, 14492.0, 14705.0, 14705.0, 14705.0, 14925.0, 14492.0, 14285.0, 14084.0, 13888.0]

# Handle None values in true_throughput
true_throughput = [float(x) if x is not None else None for x in true_throughput]

# Plotting the data
plt.figure(figsize=(10, 6))
plt.plot(thread_counts, true_throughput, marker='o', label='True Throughput')
plt.plot(thread_counts, false_throughput, marker='o', label='False Throughput')

# Adding labels and title
plt.xlabel('Thread Count')
plt.ylabel('Throughput')
plt.title('Throughput vs Thread Count')
plt.legend()
plt.grid(True)
plt.xticks(thread_counts)
plt.savefig('throughput_plot.png')
# Show the plot
plt.show()
