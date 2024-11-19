import os
import re
import pandas as pd
import sys

def extract_metrics(file_path):
    thread_match = re.search(r'hash_rt\.(\d+)\.', file_path)
    if not thread_match:
        return None
    thread_num = int(thread_match.group(1))
    
    metrics = {
        'threads': thread_num,
        'scheduler_avg_when_time_ns': None,
        'insert_avg_insert_time_ns': None
    }
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            
            scheduler_match = re.search(r'scheduler_avg_when_time_ns : ([\d.]+)', content)
            insert_match = re.search(r'insert_avg_insert_time_ns : ([\d.]+)', content)
            
            if scheduler_match:
                metrics['scheduler_avg_when_time_ns'] = float(scheduler_match.group(1))
            if insert_match:
                metrics['insert_avg_insert_time_ns'] = float(insert_match.group(1))
                
        return metrics
    except Exception as e:
        print(f"Error processing file {file_path}: {str(e)}")
        return None

def process_folder(folder_path):
    results = []
    
    for filename in os.listdir(folder_path):
        if filename.endswith('.log'):
            file_path = os.path.join(folder_path, filename)
            metrics = extract_metrics(file_path)
            if metrics:
                results.append(metrics)
    
    if results:
        df = pd.DataFrame(results)
        df = df.sort_values('threads', ascending=True)
        
        output_file = os.path.join(folder_path, 'time_metrics_sched_run.csv')
        df.to_csv(output_file, index=False)
        print(f"Results saved to: {output_file}")
    else:
        print("No valid data found")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python extract_hash_rt_time.py <log_folder_path>")
        sys.exit(1)
    
    folder_path = sys.argv[1]
    if not os.path.exists(folder_path):
        print(f"Error: Folder '{folder_path}' does not exist")
        sys.exit(1)
        
    process_folder(folder_path)