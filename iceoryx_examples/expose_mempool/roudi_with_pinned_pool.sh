#!/bin/bash

# --- begin runfiles.bash initialization v2 ---
# Copy-pasted from the Bazel Bash runfiles library v2.
set -uo pipefail; set +e; f=bazel_tools/tools/bash/runfiles/runfiles.bash
source "${RUNFILES_DIR:-/dev/null}/$f" 2>/dev/null || \
  source "$(grep -sm1 "^$f " "${RUNFILES_MANIFEST_FILE:-/dev/null}" | cut -f2- -d' ')" 2>/dev/null || \
  source "$0.runfiles/$f" 2>/dev/null || \
  source "$(grep -sm1 "^$f " "$0.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
  source "$(grep -sm1 "^$f " "$0.exe.runfiles_manifest" | cut -f2- -d' ')" 2>/dev/null || \
  { echo>&2 "ERROR: cannot find $f"; exit 1; }; f=; set -e
# --- end runfiles.bash initialization v2 ---

runfiles_export_envvars

if [[ -z "${RUNFILES_DIR}" ]]
then
    echo "Unable to find runfiles"
    exit 1
fi

# Ensure runfiles_dir is a full path.
export RUNFILES_DIR=$(realpath ${RUNFILES_DIR})
cd ${RUNFILES_DIR}/org_eclipse_iceoryx

ROUDI="iceoryx_posh/iox-roudi"
ROUDI_CONFIG="iceoryx_examples/expose_mempool/resources/pinned_pool_roudi_config.toml"

exec $ROUDI -c $ROUDI_CONFIG
