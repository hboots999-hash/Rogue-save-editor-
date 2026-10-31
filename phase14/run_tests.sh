#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
python3 phase7/create_synthetic_fixture.py "$build/one.sav"
python3 phase10/preview/make_toy_two_bank.py "$build/one.sav" "$build/two.sav"
flags=(-std=c++17 -Wall -Wextra -Wpedantic -Werror -Iphase10/preview -Iphase11 -Iphase12 -Iphase13 -Iphase14)
for mode in regular sanitized; do
  extra=(-O2)
  if [[ $mode == sanitized ]]; then extra=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie); fi
  gcc -Wall -Wextra -Werror "${extra[@]}" -c phase14/vendor/sha256.c -o "$build/sha.o"
  g++ "${flags[@]}" "${extra[@]}" phase14/test_restore.cpp "$build/sha.o" -o "$build/restore"
  ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" UBSAN_OPTIONS=halt_on_error=1 "$build/restore" "$build/two.sav"
done
for test in phase11/test_live_header.cpp phase12/test_live_backup.cpp phase13/test_restore_preflight.cpp phase13/test_live_draft.cpp; do
  g++ "${flags[@]}" -O2 "$test" -o "$build/regression"
  if [[ $test == phase11/* ]]; then "$build/regression"; else "$build/regression" "$build/one.sav"; fi
done
