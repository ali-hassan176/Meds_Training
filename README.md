# riscv-log-analyzer

Student: Ali Hassan
ID: 2024EE176
Updated learning progress

Shell-based analyzer to parse RISC-V simulation logs, summarize results, and generate reports.
This project demonstrates Linux command usage, shell scripting, Git workflows, and Makefiles.

## Requirements

- bash, grep, awk, sed, date, mktemp
- make (optional, for automation)

## Setup

```bash
make setup
chmod +x scripts/*.sh
```

## Usage

Analyze a log file (text output):

```bash
./scripts/analyze.sh test_data/sample_fail.log
```

CSV output to a file:

```bash
./scripts/analyze.sh test_data/sample_fail.log --format csv --output output/sample_fail.csv
```

Generate an HTML report across all logs:

```bash
./scripts/generate_report.sh output/report.html
```

## Make targets

```bash
make all
make test
make report
make clean
make help
```

## Sample output (text)

```text
=== RISC-V Simulation Log Analysis ===
Log file: test_data/sample_fail.log
Analysis date: 2026-05-05 14:30:00

--- Results Summary ---
Total tests: 4
Passed:           2 ( 50.0%)
Failed:           2 ( 50.0%)
Skipped:          0 (  0.0%)

--- Failed Tests ---
  1. rv32i-sll
  2. rv32i-beq
```
