# Use the official Ubuntu 18.04 image as a base image
FROM ubuntu:18.04

# Set the architecture to amd64
ARG TARGETARCH=amd64

# Set environment variables for non-interactive installation
ENV DEBIAN_FRONTEND=noninteractive

# Create a non-root user and set up home directory
RUN useradd -ms /bin/bash myuser

# Switch to the new user for the remaining commands
USER myuser

# Update the package list and install required packages as root
USER root
RUN apt-get update && apt-get install -y \
    software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update && apt-get install -y \
    gcc-7 g++-7 make cmake gdb git \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set gcc-7 as the default gcc
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 60

# Verify installation
RUN gcc --version && g++ --version && make --version && cmake --version && gdb --version

# Switch back to the non-root user
USER myuser

# Set the default command to bash
CMD ["bash"]
