---
marp: true
theme: default
paginate: true
---
# UOBP Benchmark Sweep

Date: 1 May 2026

Command template:

- go run uobp_sim.go -drop <d> -reorder <r> -latency 1 -jitter <j> -seed <s>
- 12 network conditions x 5 seeds = 60 runs
- Per-run timeout guard: 20s

---
# Headline Findings

- Total runs: 60
- Completed: 60
- Stalls/errors: 0
- Best average completion time: 76.2 ms at drop=0, reorder=0, jitter=0
- Worst average completion time: 677.4 ms at drop=0.05, reorder=0.1, jitter=5
- Worst single completion time: 1218 ms at drop=0.05, reorder=0.1, jitter=5

---
# Condition Summary

| drop | reorder | jitter | runs | ok | stall | avg_elapsed_ms | max_elapsed_ms | avg_retrans | avg_ack_packets | avg_data_sent |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 0 | 0 | 5 | 5 | 0 | 76.2 | 88 | 62.8 | 26.6 | 110.8 |
| 0 | 0.05 | 2 | 5 | 5 | 0 | 142.2 | 170 | 150.2 | 33.2 | 198.2 |
| 0 | 0.1 | 5 | 5 | 5 | 0 | 453.0 | 591 | 292.8 | 56.4 | 340.8 |
| 0.01 | 0 | 2 | 5 | 5 | 0 | 155.8 | 210 | 157.6 | 37.4 | 205.6 |
| 0.01 | 0.05 | 5 | 5 | 5 | 0 | 498.4 | 893 | 245.4 | 58.2 | 293.4 |
| 0.01 | 0.1 | 5 | 5 | 5 | 0 | 593.6 | 859 | 280.2 | 65.0 | 328.2 |

---
# Condition Summary (Cont.)

| drop | reorder | jitter | runs | ok | stall | avg_elapsed_ms | max_elapsed_ms | avg_retrans | avg_ack_packets | avg_data_sent |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0.05 | 0 | 2 | 5 | 5 | 0 | 217.8 | 300 | 148.4 | 39.8 | 196.4 |
| 0.05 | 0.05 | 2 | 5 | 5 | 0 | 218.6 | 367 | 135.8 | 40.2 | 183.8 |
| 0.05 | 0.1 | 5 | 5 | 5 | 0 | 677.4 | 1218 | 279.0 | 73.8 | 327.0 |
| 0.1 | 0 | 2 | 5 | 5 | 0 | 225.6 | 307 | 125.6 | 39.8 | 173.6 |
| 0.1 | 0.05 | 5 | 5 | 5 | 0 | 582.8 | 760 | 204.8 | 63.8 | 252.8 |
| 0.1 | 0.1 | 5 | 5 | 5 | 0 | 578.4 | 745 | 150.2 | 63.2 | 198.2 |

---
# Notes

- Results are deterministic per seed with current simulator logic.
- Higher reorder and jitter increased completion time and retransmit volume.
- No non-completing seed was observed in this sweep.
- Raw run data: benchmark_results.csv
- Aggregated data: benchmark_summary.csv
