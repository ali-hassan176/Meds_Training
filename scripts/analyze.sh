#!/usr/bin/env bash
set -euo pipefail

usage() {
	cat <<'EOF'
Usage: analyze.sh <log_file> [--format text|csv] [--output <path>] [--verbose]

Arguments:
	log_file            Path to the simulation log file (required)

Options:
	--format text|csv   Output format (default: text)
	--output <path>     Output file path (default: stdout)
	--verbose           Enable verbose output (text format only)
	--help              Show this help message
EOF
}

fail() {
	echo "Error: $*" >&2
	exit 2
}

log_file=""
format="text"
output_path=""
verbose=false

parse_args() {
	if [[ $# -lt 1 ]]; then
		usage
		exit 2
	fi

	log_file="$1"
	shift

	while [[ $# -gt 0 ]]; do
		case "$1" in
			--format)
				shift
				[[ $# -gt 0 ]] || fail "Missing value for --format"
				format="$1"
				if [[ "$format" != "text" && "$format" != "csv" ]]; then
					fail "Invalid format: $format"
				fi
				;;
			--output)
				shift
				[[ $# -gt 0 ]] || fail "Missing value for --output"
				output_path="$1"
				;;
			--verbose)
				verbose=true
				;;
			--help|-h)
				usage
				exit 0
				;;
			*)
				fail "Unknown option: $1"
				;;
		esac
		shift
	done
}

total=0
pass=0
fail_count=0
skip=0
time_count=0
time_sum=0
min_time=""
min_name=""
max_time=""
max_name=""
fail_list=""
time_list=""

collect_stats() {
	local stats_file
	stats_file="$(mktemp)"

	awk -v OFS='\t' '
	BEGIN {
		total = 0
		pass = 0
		fail = 0
		skip = 0
		time_count = 0
		sum = 0
	}
	function record_time(name, time) {
		time_count++
		sum += time
		if (time_count == 1 || time < min_time) { min_time = time; min_name = name }
		if (time_count == 1 || time > max_time) { max_time = time; max_name = name }
		time_list = (time_list == "" ? name "=" time : time_list "|" name "=" time)
	}
	function add_fail(name) {
		fail_list = (fail_list == "" ? name : fail_list "|" name)
	}
	function extract_name(line) {
		name = line
		sub(/.*TEST (PASS|FAIL|SKIP):[[:space:]]*/, "", name)
		sub(/[[:space:]].*$/, "", name)
		return name
	}
	function extract_time(line) {
		time = ""
		if (match(line, /\([0-9.]+s\)/)) {
			time = substr(line, RSTART + 1, RLENGTH - 2)
			sub(/s$/, "", time)
		}
		return time
	}
	{
		if ($0 ~ /TEST PASS:/) {
			total++
			pass++
			name = extract_name($0)
			time = extract_time($0)
			if (time != "") { record_time(name, time) }
		} else if ($0 ~ /TEST FAIL:/) {
			total++
			fail++
			name = extract_name($0)
			add_fail(name)
			time = extract_time($0)
			if (time != "") { record_time(name, time) }
		} else if ($0 ~ /TEST SKIP:/) {
			total++
			skip++
		}
	}
	END {
		print "total", total
		print "pass", pass
		print "fail", fail
		print "skip", skip
		print "time_count", time_count
		print "time_sum", sum
		print "min_time", min_time
		print "min_name", min_name
		print "max_time", max_time
		print "max_name", max_name
		print "fail_list", fail_list
		print "time_list", time_list
	}' "$log_file" > "$stats_file"

	while IFS=$'\t' read -r key value; do
		case "$key" in
			total) total="$value" ;;
			pass) pass="$value" ;;
			fail) fail_count="$value" ;;
			skip) skip="$value" ;;
			time_count) time_count="$value" ;;
			time_sum) time_sum="$value" ;;
			min_time) min_time="$value" ;;
			min_name) min_name="$value" ;;
			max_time) max_time="$value" ;;
			max_name) max_name="$value" ;;
			fail_list) fail_list="$value" ;;
			time_list) time_list="$value" ;;
		esac
	done < "$stats_file"

	rm -f "$stats_file"
}

format_rate() {
	awk -v part="$1" -v total="$2" 'BEGIN { if (total == 0) printf "0.0"; else printf "%.1f", (part / total) * 100 }'
}

format_time() {
	printf "%.2f" "$1"
}

render_text() {
	local analysis_date pass_rate fail_rate skip_rate avg_time verdict exit_code
	analysis_date="$(date '+%Y-%m-%d %H:%M:%S')"
	pass_rate="$(format_rate "$pass" "$total")"
	fail_rate="$(format_rate "$fail_count" "$total")"
	skip_rate="$(format_rate "$skip" "$total")"

	avg_time="NA"
	if [[ "$time_count" -gt 0 ]]; then
		avg_time="$(awk -v sum="$time_sum" -v count="$time_count" 'BEGIN { printf "%.2f", sum / count }')"
	fi

	if [[ "$fail_count" -eq 0 ]]; then
		verdict="PASS"
		exit_code=0
	else
		verdict="FAIL"
		exit_code=1
	fi

	printf "=== RISC-V Simulation Log Analysis ===\n"
	printf "Log file: %s\n" "$log_file"
	printf "Analysis date: %s\n\n" "$analysis_date"

	printf '%s\n' '--- Results Summary ---'
	printf "Total tests: %s\n" "$total"
	printf "Passed:  %10s (%5s%%)\n" "$pass" "$pass_rate"
	printf "Failed:  %10s (%5s%%)\n" "$fail_count" "$fail_rate"
	printf "Skipped: %10s (%5s%%)\n\n" "$skip" "$skip_rate"

	printf '%s\n' '--- Failed Tests ---'
	if [[ -z "$fail_list" ]]; then
		printf "None\n\n"
	else
		local idx=1
		IFS='|' read -r -a fail_items <<< "$fail_list"
		for item in "${fail_items[@]}"; do
			printf "  %d. %s\n" "$idx" "$item"
			idx=$((idx + 1))
		done
		printf "\n"
	fi

	printf '%s\n' '--- Timing Per Test ---'
	if [[ -z "$time_list" ]]; then
		printf "No timing data found.\n\n"
	else
		IFS='|' read -r -a time_items <<< "$time_list"
		for item in "${time_items[@]}"; do
			local name="${item%%=*}"
			local time_value="${item#*=}"
			printf "  %s: %ss\n" "$name" "$(format_time "$time_value")"
		done
		printf "\n"
	fi

	printf '%s\n' '--- Timing Statistics ---'
	if [[ "$time_count" -gt 0 ]]; then
		printf "Min time: %ss (%s)\n" "$(format_time "$min_time")" "$min_name"
		printf "Max time: %ss (%s)\n" "$(format_time "$max_time")" "$max_name"
		printf "Avg time: %ss\n\n" "$avg_time"
	else
		printf "Min time: NA\nMax time: NA\nAvg time: NA\n\n"
	fi

	printf '%s\n' "--- Verdict: $verdict ---"
	printf "Exit code: %s\n" "$exit_code"

	if [[ "$verbose" == true ]]; then
		printf '%s\n' "" "--- Matched Log Lines ---"
		grep -n "TEST " "$log_file" || true
	fi
}

render_csv() {
	local pass_rate avg_time min_out max_out fail_out
	pass_rate="$(format_rate "$pass" "$total")"
	avg_time="NA"
	min_out="NA"
	max_out="NA"

	if [[ "$time_count" -gt 0 ]]; then
		avg_time="$(awk -v sum="$time_sum" -v count="$time_count" 'BEGIN { printf "%.2f", sum / count }')"
		min_out="$(format_time "$min_time")"
		max_out="$(format_time "$max_time")"
	fi

	fail_out="$fail_list"
	printf "log_file,total,passed,failed,skipped,pass_rate,min_time,min_time_test,max_time,max_time_test,avg_time,failed_tests\n"
	printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" \
		"$log_file" "$total" "$pass" "$fail_count" "$skip" "$pass_rate" \
		"$min_out" "$min_name" "$max_out" "$max_name" "$avg_time" "$fail_out"
}

main() {
	parse_args "$@"

	if [[ ! -f "$log_file" ]]; then
		fail "Log file not found: $log_file"
	fi

	collect_stats

	if [[ -n "$output_path" ]]; then
		mkdir -p "$(dirname "$output_path")"
	fi

	if [[ "$format" == "text" ]]; then
		if [[ -n "$output_path" ]]; then
			render_text > "$output_path"
		else
			render_text
		fi
	else
		if [[ -n "$output_path" ]]; then
			render_csv > "$output_path"
		else
			render_csv
		fi
	fi

	if [[ "$fail_count" -eq 0 ]]; then
		exit 0
	fi
	exit 1
}

main "$@"
