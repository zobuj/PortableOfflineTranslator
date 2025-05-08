import pandas as pd
import matplotlib.pyplot as plt

# Load the raw CSV
with open("pipeline_mem_log.csv") as f:
    lines = f.readlines()

# Parse header and lines
header = lines[0].strip().split(",")
rows = []

for line in lines[1:]:
    parts = line.strip().split(",")
    if len(parts) != 2:
        continue  # skip malformed lines
    timestamp = int(parts[0].strip())
    # Split the second part into rss and vsz
    mem_parts = parts[1].strip().split()
    if len(mem_parts) != 2:
        continue  # skip if memory data is malformed
    rss_kb = int(mem_parts[0])
    vsz_kb = int(mem_parts[1])
    rows.append((timestamp, rss_kb, vsz_kb))

# Convert to DataFrame
df = pd.DataFrame(rows, columns=["timestamp", "rss_kb", "vsz_kb"])

# Relative time and MB conversion
df["time_s"] = df["timestamp"] - df["timestamp"].iloc[0]
df["rss_mb"] = df["rss_kb"] / 1024
df["vsz_mb"] = df["vsz_kb"] / 1024

# Plot
plt.figure()
plt.plot(df["time_s"], df["rss_mb"], label="RSS (MB)")
plt.plot(df["time_s"], df["vsz_mb"], label="VSZ (MB)", linestyle="--")
plt.xlabel("Time (s)")
plt.ylabel("Memory (MB)")
plt.title("Pipeline Memory Usage Over Time")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("pipeline_memory_usage_high.png")
import pandas as pd
import matplotlib.pyplot as plt

# Load the raw CSV
with open("pipeline_mem_log.csv") as f:
    lines = f.readlines()

# Parse header and lines
header = lines[0].strip().split(",")
rows = []

for line in lines[1:]:
    parts = line.strip().split(",")
    if len(parts) != 2:
        continue  # skip malformed lines
    timestamp = int(parts[0].strip())
    # Split the second part into rss and vsz
    mem_parts = parts[1].strip().split()
    if len(mem_parts) != 2:
        continue  # skip if memory data is malformed
    rss_kb = int(mem_parts[0])
    vsz_kb = int(mem_parts[1])
    rows.append((timestamp, rss_kb, vsz_kb))

# Convert to DataFrame
df = pd.DataFrame(rows, columns=["timestamp", "rss_kb", "vsz_kb"])

# Relative time and MB conversion
df["time_s"] = df["timestamp"] - df["timestamp"].iloc[0]
df["rss_mb"] = df["rss_kb"] / 1024
df["vsz_mb"] = df["vsz_kb"] / 1024

# Plot
plt.figure()
plt.plot(df["time_s"], df["rss_mb"], label="RSS (MB)")
plt.plot(df["time_s"], df["vsz_mb"], label="VSZ (MB)", linestyle="--")
plt.xlabel("Time (s)")
plt.ylabel("Memory (MB)")
plt.title("Pipeline Memory Usage Over Time")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("pipeline_memory_usage_high.png")
