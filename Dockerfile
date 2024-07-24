# Use the official base image of Ubuntu 18.04 (Bionic Beaver)
FROM ubuntu:18.04

# Set environment variables to avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update the package list and install necessary packages
RUN apt-get update && \
    apt-get install -y gcc make && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /workspace

# Copy all files to the working directory
COPY . .

# Compile the C program using make
RUN make

# Run the compiled program
CMD ["./build/manster"]
