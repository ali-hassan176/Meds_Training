#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
data_dir="$root_dir/test_data"
output_dir="$root_dir/output"
analyzer="$root_dir/scripts/analyze.sh"

output_file="${1:-$output_dir/report.html}"

if [[ ! -x "$analyzer" ]]; then
	echo "Error: analyzer script not executable at $analyzer" >&2
	exit 2
fi

mkdir -p "$output_dir"

csv_tmp="$(mktemp)"
header_written=false

if ! compgen -G "$data_dir/*.log" > /dev/null; then
	echo "Error: no .log files found in $data_dir" >&2
	exit 2
fi

for log_file in "$data_dir"/*.log; do
	set +e
	csv_out="$("$analyzer" "$log_file" --format csv)"
	status=$?
	set -e
	if [[ "$status" -eq 2 ]]; then
		echo "Error: analyzer failed for $log_file" >&2
		exit 2
	fi
	if [[ "$header_written" == false ]]; then
		printf '%s\n' "$csv_out" > "$csv_tmp"
		header_written=true
	else
		printf '%s\n' "$csv_out" | tail -n +2 >> "$csv_tmp"
	fi
done

{
	echo "<!doctype html>"
	echo "<html lang=\"en\">"
	echo "<head>"
	echo "  <meta charset=\"utf-8\">"
	echo "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
	echo "  <title>RISC-V Log Analysis Report</title>"
	echo "  <style>"
	echo "    body { font-family: Arial, sans-serif; margin: 24px; color: #222; }"
	echo "    h1 { font-size: 20px; }"
	echo "    table { border-collapse: collapse; width: 100%; }"
	echo "    th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }"
	echo "    th { background: #f4f4f4; }"
	echo "    .fail { color: #b30000; font-weight: bold; }"
	echo "    .pass { color: #0a7a31; font-weight: bold; }"
	echo "  </style>"
	echo "</head>"
	echo "<body>"
	echo "  <h1>RISC-V Simulation Log Report</h1>"
	echo "  <p>Generated: $(date '+%Y-%m-%d %H:%M:%S')</p>"
	echo "  <table>"
	echo "    <thead>"
	echo "      <tr>"
	echo "        <th>Log File</th>"
	echo "        <th>Total</th>"
	echo "        <th>Passed</th>"
	echo "        <th>Failed</th>"
	echo "        <th>Skipped</th>"
	echo "        <th>Pass Rate (%)</th>"
	echo "        <th>Min Time (s)</th>"
	echo "        <th>Min Test</th>"
	echo "        <th>Max Time (s)</th>"
	echo "        <th>Max Test</th>"
	echo "        <th>Avg Time (s)</th>"
	echo "        <th>Failed Tests</th>"
	echo "      </tr>"
	echo "    </thead>"
	echo "    <tbody>"

	while IFS=',' read -r log_name total passed failed skipped pass_rate min_time min_test max_time max_test avg_time failed_tests; do
		if [[ "$log_name" == "log_file" ]]; then
			continue
		fi
		if [[ -z "$failed_tests" ]]; then
			failed_tests="none"
		fi
		verdict_class="pass"
		if [[ "$failed" != "0" ]]; then
			verdict_class="fail"
		fi
		echo "      <tr>"
		echo "        <td>$log_name</td>"
		echo "        <td>$total</td>"
		echo "        <td>$passed</td>"
		echo "        <td class=\"$verdict_class\">$failed</td>"
		echo "        <td>$skipped</td>"
		echo "        <td>$pass_rate</td>"
		echo "        <td>$min_time</td>"
		echo "        <td>$min_test</td>"
		echo "        <td>$max_time</td>"
		echo "        <td>$max_test</td>"
		echo "        <td>$avg_time</td>"
		echo "        <td>$failed_tests</td>"
		echo "      </tr>"
	done < "$csv_tmp"

	echo "    </tbody>"
	echo "  </table>"
	echo "</body>"
	echo "</html>"
} > "$output_file"

rm -f "$csv_tmp"

echo "HTML report written to $output_file"
