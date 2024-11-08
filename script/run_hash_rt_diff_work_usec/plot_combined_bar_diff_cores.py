import pandas as pd
import matplotlib.pyplot as plt
import os
import argparse

def plot_time_analysis(datetime_str):
    base_path = "/users/Xuran/hash_rt/log"
    datetime_path = os.path.join(base_path, datetime_str)
    
    if not os.path.exists(datetime_path):
        print(f"Error: Directory not found {datetime_path}")
        return
        
    subdirs = [d for d in os.listdir(datetime_path) if os.path.isdir(os.path.join(datetime_path, d))]
    
    for subdir in subdirs:
        file_path = os.path.join(datetime_path, subdir, "detailed_time_analysis.csv")
        if not os.path.exists(file_path):
            continue
            
        df = pd.read_csv(file_path)
        
        # Convert nanoseconds to milliseconds
        time_columns = ['init_scheduler_time', 'schedule_behaviours_time', 'run_behaviours_time']
        for col in time_columns:
            df[col] = df[col] / 1e6
            
        plt.figure(figsize=(14, 8))
        
        plt.rcParams.update({'font.size': 14})
        
        bar_width = 0.6
        x = range(len(df['cores']))
        
        plt.bar(x, df['init_scheduler_time'], width=bar_width, label='Init Scheduler')
        plt.bar(x, df['schedule_behaviours_time'], width=bar_width, 
               bottom=df['init_scheduler_time'], label='Schedule Behaviours')
        plt.bar(x, df['run_behaviours_time'], width=bar_width, 
               bottom=df['init_scheduler_time'] + df['schedule_behaviours_time'], 
               label='Run Behaviours')
        
        plt.xlabel('Hardware Threads', fontsize=14)
        plt.ylabel('Time Taken (ms)', fontsize=14)
        plt.title(f'Time Distribution Across Different Components with Busyloop ({subdir})', 
                 fontsize=16)
        plt.legend(fontsize=12)
        plt.xticks(fontsize=12)
        plt.yticks(fontsize=12)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        
        save_path = os.path.join(datetime_path, subdir, 'time_analysis.png')
        plt.savefig(save_path)
        print(f"Chart saved to: {save_path}")
        plt.close()

def main():
    parser = argparse.ArgumentParser(description='Generate time analysis charts from CSV data')
    parser.add_argument('datetime', type=str, help='Date and time string (format: YYYY-MM-DD-HH-MM-SS)')
    
    args = parser.parse_args()
    plot_time_analysis(args.datetime)

if __name__ == "__main__":
    main()
