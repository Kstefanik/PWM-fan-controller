FROM ghcr.io/zephyrproject-rtos/zephyr-build:main

# Switch to root to install packages and manage system directories
USER root

# Install basic tools, USB support, OpenOCD, ST-Link, and X11/GUI dependencies for Ozone
RUN apt-get update && apt-get install -y \
    wget \
    curl \
    unzip \
    clangd \
    && rm -rf /var/lib/apt/lists/*

# Pre-create the workdir and ensure the non-root 'user' owns it
# This prevents the named volume from defaulting to root ownership
RUN mkdir -p /workdir && chown -R user:user /workdir

# Switch back to the default user in the Zephyr base image
USER user

# Set the default working directory
WORKDIR /workdir/PWM-fan-controller