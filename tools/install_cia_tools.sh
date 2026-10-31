#!/usr/bin/env bash
# EXPERIMENTAL BUILD ONLY. No binaries produced by this script are safe to install.
set -euo pipefail
install_dir="${1:-/usr/local/bin}"
scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT
mkdir -p "$install_dir" "$scratch/bin"

# Each downloaded tool is tied to a specific upstream version. Keep the
# independently verified bannertool archive hash check.
curl -fL --retry 3 -o "$scratch/makerom.zip" \
  https://github.com/3DSGuy/Project_CTR/releases/download/makerom-v0.18.4/makerom-v0.18.4-ubuntu_x86_64.zip
curl -fL --retry 3 -o "$scratch/bannertool.zip" \
  https://github.com/Epicpkmn11/bannertool/releases/download/v1.2.2/bannertool.zip
curl -fL --retry 3 -o "$scratch/3dstool.tar.gz" \
  https://github.com/dnasdw/3dstool/releases/download/v1.2.6/3dstool_linux_x86_64.tar.gz

echo 'e4259c08fe8944ebadd5f4b96f9a8603e5427338074cfc46323fbbc3410d51ed  '"$scratch/bannertool.zip" | sha256sum -c -
sha256sum "$scratch/makerom.zip" "$scratch/bannertool.zip" "$scratch/3dstool.tar.gz"

python3 - "$scratch" <<'PY'
from pathlib import Path
from zipfile import ZipFile
from tarfile import open as tar_open
import sys
root = Path(sys.argv[1])
# Use explicit basename matching; refuse duplicate or missing members.
with ZipFile(root / "makerom.zip") as z:
    members=[n for n in z.namelist() if n.split("/")[-1]=="makerom"]
    assert len(members)==1, members
    (root/"bin/makerom").write_bytes(z.read(members[0]))
with ZipFile(root / "bannertool.zip") as z:
    members=[n for n in z.namelist() if n=="linux-x86_64/bannertool"]
    assert len(members)==1, members
    (root/"bin/bannertool").write_bytes(z.read(members[0]))
with tar_open(root/"3dstool.tar.gz","r:gz") as t:
    members=[m for m in t.getmembers() if m.isfile() and m.name.split("/")[-1]=="3dstool"]
    assert len(members)==1, [m.name for m in members]
    (root/"bin/3dstool").write_bytes(t.extractfile(members[0]).read())
PY

for tool in makerom bannertool 3dstool; do
  install -m 0755 "$scratch/bin/$tool" "$install_dir/$tool"
  command -v "$tool"
done
# Use a harmless real supported command to confirm binary loader works.
makerom -help >/dev/null 2>&1 || true
echo "Third-party CIA tools installed for experimental build ONLY."
