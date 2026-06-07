FROM ghcr.io/zephyrproject-rtos/zephyr-build:main

USER root

RUN apt-get update && apt-get install -y \
    clangd \
    clang \
    clang-tidy \
    cppcheck \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Pre-create workdir with the right owner
RUN mkdir -p /workdir && chown -R user:user /workdir

#Install required python packages
RUN pip3 install --no-cache-dir codechecker
RUN pip3 install --no-cache-dir "zcbor==0.9.1"
RUN pip3 install --no-cache-dir "cbor2<6.0.0" # required for zcbor 0.9.1

# Create python venv dir for in-container intellisense for PC app
RUN mkdir -p /opt/venv-python-intellisense && chown -R user:user /opt/venv-python-intellisense

USER user

# Create venv for PC app intellisense
RUN python3 -m venv /opt/venv-python-intellisense

WORKDIR /workdir/PWM-fan-controller