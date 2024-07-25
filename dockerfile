# Use the official Ubuntu 18.04 image as a base image
FROM ubuntu:18.04

# Set the architecture to amd64
ARG TARGETARCH=amd64

# Set environment variables for non-interactive installation
ENV DEBIAN_FRONTEND=noninteractive

# Update the package list and install required packages
RUN apt-get update && apt-get install -y \
    software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update && apt-get install -y \
    gcc-7 g++-7 make cmake gdb \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set gcc-7 as the default gcc
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 60

# Verify installation
RUN gcc --version && g++ --version && make --version && cmake --version && gdb --version

# Set the default command to bash
CMD ["bash"]
