### apache_server_test.py

import subprocess
import pandas as pd
import json
from tqdm import tqdm
import time
import os
def run_apache_server_test():
    BASE_URL = "http://localhost:8080/"
    ENDPOINT = "/index.html"
    CONNECTIONS = 500
    RATES = list(range(500, 1000, 100))
    RUNS_PER_RATE = 5
    TEST_DURATION = 5
    CSV_FILE = "apache_results.csv"
    if os.path.exists(CSV_FILE):
        os.remove(CSV_FILE)
    try:
        pd.read_csv(CSV_FILE)
    except FileNotFoundError:
        pd.DataFrame(columns=["Request Rate (RPS)", "Avg Latency (ms)", "Avg Throughput (req/s)"]).to_csv(CSV_FILE, index=False)

    for rate in RATES:
        total_latency = 0
        total_throughput = 0
        successful_runs = 0

        print(f"\n=== [apache-Driven Server] Testing RPS: {rate} ===")

        for run_num in range(1, RUNS_PER_RATE + 1):
            with tqdm(total=TEST_DURATION, desc=f"Run {run_num}/{RUNS_PER_RATE} @ {rate} RPS") as pbar:
                try:
                    command = f"echo \"GET {BASE_URL}{ENDPOINT}\" | vegeta attack -insecure -rate={rate} -duration={TEST_DURATION}s -connections={CONNECTIONS} | vegeta report -type=json"
                    proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                    for _ in range(TEST_DURATION):
                        time.sleep(1)
                        pbar.update(1)

                    stdout, _ = proc.communicate()
                    result = json.loads(stdout.decode())

                    if 'throughput' in result and 'latencies' in result and 'mean' in result['latencies']:
                        total_throughput += result['throughput']
                        total_latency += result['latencies']['mean']
                        successful_runs += 1

                except Exception:
                    continue

        if successful_runs > 0:
            avg_latency_ms = (total_latency / successful_runs) / 1e6
            avg_throughput = total_throughput / successful_runs
        else:
            avg_latency_ms = 0
            avg_throughput = 0

        df_new = pd.DataFrame({
            "Request Rate (RPS)": [rate],
            "Avg Latency (ms)": [avg_latency_ms],
            "Avg Throughput (req/s)": [avg_throughput]
        })

        df_existing = pd.read_csv(CSV_FILE)
        df_combined = pd.concat([df_existing, df_new], ignore_index=True)
        df_combined.to_csv(CSV_FILE, index=False)

if __name__ == "__main__":
    run_apache_server_test()


