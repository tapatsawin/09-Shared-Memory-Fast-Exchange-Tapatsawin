#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

make clean >/dev/null 2>&1 || true
make >/dev/null 2>&1

output="$(./bin/shared_memory_lab 2>&1 || true)"

echo "$output"

# Reject unimplemented stubs
if echo "$output" | grep -q "TODO:"; then
    echo "visible test FAILED: output contains TODO placeholder text" >&2
    exit 1
fi

expected="child fast read: shared memory is fast
parent fast write: done
child latest view: latest-only
limit observed: no history in shared memory
child capacity bytes: 15
parent source bytes: 26
all tasks done"

if [[ "$output" != "$expected" ]]; then
    echo "visible test FAILED: output does not match expected" >&2
    echo "--- expected ---" >&2
    echo "$expected" >&2
    echo "--- got ---" >&2
    echo "$output" >&2
    exit 1
fi

echo "visible test: passed"