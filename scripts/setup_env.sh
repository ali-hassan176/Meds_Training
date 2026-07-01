#!/usr/bin/env bash
set -euo pipefail

required_tools=(bash grep awk sed date mktemp)
missing_tools=()

for tool in "${required_tools[@]}"; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		missing_tools+=("$tool")
	fi
done

if [[ "${#missing_tools[@]}" -gt 0 ]]; then
	echo "Missing required tools: ${missing_tools[*]}" >&2
	exit 2
fi

echo "All required tools are available."
