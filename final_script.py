import pandas as pd
import matplotlib.pyplot as plt

files = {
    "Event-Driven Server": "event_results.csv",
    "Thread-Based Server": "thread_results.csv",
    "Apache Server": "apache_results.csv"
}

latency_data = {}
throughput_data = {}

for server, file in files.items():
    df = pd.read_csv(file)
    latency_data[server] = df["Avg Latency (ms)"]
    throughput_data[server] = df["Avg Throughput (req/s)"]
    rates = df["Request Rate (RPS)"]

plt.figure()
for server, data in latency_data.items():
    plt.plot(rates, data, marker='o', label=server)
plt.xlabel("Request Rate (RPS)")
plt.ylabel("Avg Latency (ms)")
plt.title("Latency Comparison")
plt.legend()
plt.grid(True)
plt.savefig("latency_comparison.png")

plt.figure()
for server, data in throughput_data.items():
    plt.plot(rates, data, marker='o', label=server)
plt.xlabel("Request Rate (RPS)")
plt.ylabel("Avg Throughput (req/s)")
plt.title("Throughput Comparison")
plt.legend()
plt.grid(True)
plt.savefig("throughput_comparison.png")

print("\n✅ Plots saved: 'latency_comparison.png' and 'throughput_comparison.png'")