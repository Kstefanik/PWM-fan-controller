#!/usr/bin/env bash

WORKSPACE="/workdir/PWM-fan-controller"

run() {
  echo "+ $*"
  "$@"
}

# Initailze west workspace and update west
if [ ! -d "$WORKSPACE/.west" ]; then
  run west init -l "$WORKSPACE/firmware"
else
  echo "Workspace already initialized, skipping..."
fi

run west update
run west zephyr-export

# Install PC app requirements for in-container intellisense
run /opt/venv-python-intellisense/bin/pip install --no-cache-dir -r /workdir/PWM-fan-controller/app/requirements.txt

# Correct eventual permission errors for ccache
run sudo chown -R user:user /home/user/.ccache