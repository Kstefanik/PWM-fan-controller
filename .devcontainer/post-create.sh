#!/usr/bin/env bash
set -euo pipefail

# Resolve workspace from this script location: <workspace>/.devcontainer/post-create.sh
WORKSPACE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST_DIR="firmware"
MANIFEST_FILE="west.yml"

run() {
  echo "+ $*"
  "$@"
}

cd "$WORKSPACE"

# Add safe.directory only if it is missing.
if ! git config --global --get-all safe.directory 2>/dev/null | grep -Fxq "$WORKSPACE"; then
  run git config --global --add safe.directory "$WORKSPACE"
fi

# Ensure this repository is its own west topdir (T1 layout).
# A parent .west can be detected and cause the local setup to be skipped.
if [ ! -d "$WORKSPACE/.west" ]; then
  # run west init -l "$WORKSPACE/$MANIFEST_DIR" "$WORKSPACE"
  run west init -l "$WORKSPACE/$MANIFEST_DIR"
fi

run west config --local manifest.path "$MANIFEST_DIR"
run west config --local manifest.file "$MANIFEST_FILE"

if [ "$(west topdir)" != "$WORKSPACE" ]; then
  echo "ERROR: west topdir is '$(west topdir)', expected '$WORKSPACE'"
  echo "Hint: remove or reconfigure parent workspace metadata (for example /workdir/.west)."
  exit 1
fi

run west update
run west zephyr-export
