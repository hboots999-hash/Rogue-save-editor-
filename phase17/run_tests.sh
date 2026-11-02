#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
python3 phase7/create_synthetic_fixture.py "$tmp/one.sav"
python3 phase10/preview/make_toy_two_bank.py "$tmp/one.sav" "$tmp/two.sav"
flags=(-std=c++17 -Wall -Wextra -Wpedantic -Werror
       -Iphase10/preview -Iphase11 -Iphase13 -Iphase14 -Iphase15 -Iphase16 -Iphase17)
for mode in strict sanitized; do
  extra=(-O2)
  if [[ $mode == sanitized ]]; then
    extra=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie)
  fi
  gcc -Wall -Wextra -Werror "${extra[@]}" -c phase14/vendor/sha256.c -o "$tmp/sha.o"
  g++ "${flags[@]}" "${extra[@]}" -c phase17/readonly_status_v1.cpp -o "$tmp/status.o"
  g++ "${flags[@]}" "${extra[@]}" phase17/test_rollback_lab.cpp "$tmp/status.o" "$tmp/sha.o" -o "$tmp/lab"
  ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" UBSAN_OPTIONS=halt_on_error=1 \
    "$tmp/lab" "$tmp/two.sav"
  gcc -std=c11 -Wall -Wextra -Wpedantic -Werror "${extra[@]}" -Iphase17 \
    -c phase17/test_status_abi_c.c -o "$tmp/c-abi.o"
  g++ "${extra[@]}" "$tmp/c-abi.o" "$tmp/status.o" -o "$tmp/c-abi"
  ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" UBSAN_OPTIONS=halt_on_error=1 "$tmp/c-abi"
  # Deliberately separate from the actual Phase20 editor: isolated offline
  # SD recovery evidence review has no file IO, privileged archive or writer.
  g++ "${flags[@]}" "${extra[@]}" phase17/test_offline_sd_recovery_audit.cpp \
    "$tmp/sha.o" -o "$tmp/sd-review"
  ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" UBSAN_OPTIONS=halt_on_error=1 \
    "$tmp/sd-review"
done
bash phase16/run_tests.sh
