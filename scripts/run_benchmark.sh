#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
COUNT="${COUNT:-1000000}"
SEED="${SEED:-1}"
TMPFILE=$(mktemp /tmp/lob_flow.XXXXXX.txt)
trap 'rm -f "$TMPFILE"' EXIT

echo "Generating ${COUNT} events (seed=${SEED}) -> ${TMPFILE}"
"${BUILD_DIR}/simulated_flow" --count "${COUNT}" --seed "${SEED}" --out "${TMPFILE}"

echo
"${BUILD_DIR}/bench_throughput" "${TMPFILE}"
