# Usage Guide

## analyze.sh

Analyze a single log file and print a summary.

```bash
./scripts/analyze.sh <log_file> [--format text|csv] [--output <path>] [--verbose]
```

Options:

- `--format text|csv` : Output format (default: text)
- `--output <path>`   : Write output to a file (default: stdout)
- `--verbose`         : Include matched test lines (text only)
- `--help`            : Show help

Exit codes:

- `0` if all tests pass
- `1` if any tests fail
- `2` for invalid usage or missing files

Examples:

```bash
./scripts/analyze.sh test_data/sample_pass.log
./scripts/analyze.sh test_data/sample_fail.log --verbose
./scripts/analyze.sh test_data/sample_sim.log --format csv --output output/sample_sim.csv
```

## generate_report.sh

Generate an HTML report for all `.log` files in `test_data/`.

```bash
./scripts/generate_report.sh [output_path]
```

If no output path is provided, the report is written to `output/report.html`.

## setup_env.sh

Verify that required CLI tools are available:

```bash
./scripts/setup_env.sh
```
