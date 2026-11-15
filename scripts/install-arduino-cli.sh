#!/usr/bin/env bash
# Installs the checksum-verified Linux x86-64 compiler frontend used by CI.
set -euo pipefail

if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
  echo 'This installer supports Linux x86-64. Install the pinned Arduino CLI version for your host separately.' >&2
  exit 2
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
destination=${1:-"$script_dir/../build-ci/bin"}
mkdir -p -- "$destination"
destination=$(cd -- "$destination" && pwd)
temporary=$(mktemp -d)
trap 'rm -rf -- "$temporary"' EXIT

readarray -t settings < <(python3 - "$script_dir/ci-targets.json" <<'PY'
import json, sys
config = json.load(open(sys.argv[1], encoding="utf-8"))["arduino_cli"]
print(config["linux_x86_64_url"])
print(config["linux_x86_64_sha256"])
PY
)
curl --fail --location --retry 3 --output "$temporary/arduino-cli.tar.gz" "${settings[0]}"
printf '%s  %s\n' "${settings[1]}" "$temporary/arduino-cli.tar.gz" | sha256sum --check --status
tar -xzf "$temporary/arduino-cli.tar.gz" -C "$destination" arduino-cli
"$destination/arduino-cli" version
if [[ -n "${GITHUB_PATH:-}" ]]; then
  printf '%s\n' "$destination" >> "$GITHUB_PATH"
fi
