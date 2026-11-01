#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
python3 qa/check_safety.py
for p in phase17 phase18 phase19; do git -C "dependencies/$p" rev-parse HEAD; done
bash dependencies/phase17/phase17/run_tests.sh
for mode in strict sanitized; do
  extra=(-O2)
  if [[ "$mode" == sanitized ]]; then extra=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie); fi
  common=(-std=c++17 -Wall -Wextra -Wpedantic -Werror)
  gcc -Wall -Wextra -Werror "${extra[@]}" -c dependencies/phase17/phase14/vendor/sha256.c -o "$tmp/sha.o"
  g++ "${common[@]}" "${extra[@]}" -Idependencies/phase17/phase17 -Idependencies/phase17/phase14 -Idependencies/phase17/phase11 -Idependencies/phase17/phase13 -Idependencies/phase17/phase10/preview qa/test_recovery_policy.cpp "$tmp/sha.o" -o "$tmp/recovery"
  "$tmp/recovery"
  for name in test_offline_engine test_offline_export test_pc_readonly test_editor_model test_integrity; do
    g++ "${common[@]}" "${extra[@]}" -Idependencies/phase18/phase18 dependencies/phase18/phase18/"$name".cpp -o "$tmp/$name"
    "$tmp/$name" "$tmp/nonplayable.sav"
  done
  g++ "${common[@]}" "${extra[@]}" -Idependencies/phase18 -Idependencies/phase18/phase18 -Idependencies/phase19 -Idependencies/phase19/phase19 dependencies/phase19/phase19/test_readonly.cpp -o "$tmp/readonly"
  "$tmp/readonly"
  g++ "${common[@]}" "${extra[@]}" -Idependencies/phase18 -Idependencies/phase18/phase18 -Idependencies/phase19 -Idependencies/phase19/phase19 qa/test_cross_branch.cpp -o "$tmp/cross"
  "$tmp/cross"
done
echo 'PASS isolated QA tests; no real game input'
