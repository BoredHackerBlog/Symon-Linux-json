#!/bin/bash
#====================================================================
# build-sysmon.sh
#
# Builds the Sysmon binary with JSON output support on Ubuntu.
#
# Usage:
#   ./build-sysmon.sh [build-dir]
#
# Default build directory: ./build
#====================================================================
set -euo pipefail

BUILD_DIR="${1:-build}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "================================================================"
echo "Sysmon-Linux JSON Build Script"
echo "================================================================"
echo "Source directory: $SCRIPT_DIR"
echo "Build directory:  $BUILD_DIR"
echo ""

#--------------------------------------------------------------------
# Check we're running as a non-root user with sudo access
#--------------------------------------------------------------------
if [[ $EUID -eq 0 ]]; then
    echo "ERROR: Do not run this script as root. Run as a normal user with sudo."
    exit 1
fi

if ! sudo -n true 2>/dev/null; then
    echo "ERROR: This script requires sudo access."
    exit 1
fi

#--------------------------------------------------------------------
# Step 1: Install build dependencies
#--------------------------------------------------------------------
echo "------------------------------------------------------------"
echo "Step 1: Installing build dependencies"
echo "------------------------------------------------------------"

sudo apt-get update -qq

# Microsoft package repository (for sysinternalsebpf and dotnet-sdk)
if [[ ! -f /etc/apt/sources.list.d/packages-microsoft-prod.list ]]; then
    echo "Adding Microsoft package repository..."
    wget -q "https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb" \
        -O /tmp/packages-microsoft-prod.deb
    sudo dpkg -i /tmp/packages-microsoft-prod.deb
    sudo apt-get update -qq
fi

sudo apt-get install -y \
    build-essential \
    autoconf \
    automake \
    libtool \
    git \
    cmake \
    libjson-c-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    pkg-config \
    libcap-dev \
    libpopt-dev \
    libelf-dev \
    libzstd1 \
    llvm \
    clang \
    libgtest-dev \
    googletest \
    google-mock \
    libgmock-dev \
    sysinternalsebpf \
    dotnet-sdk-10.0

#--------------------------------------------------------------------
# Step 2: Install dotnet-t4 tool (required for code generation)
#--------------------------------------------------------------------
echo "------------------------------------------------------------"
echo "Step 2: Installing dotnet-t4 tool"
echo "------------------------------------------------------------"

if ! command -v t4 &>/dev/null; then
    dotnet tool install --global dotnet-t4 --version 2.3.1
fi

export PATH="$PATH:$HOME/.dotnet/tools"

if ! command -v t4 &>/dev/null; then
    echo "ERROR: t4 tool not found after installation. Check ~/.dotnet/tools is in PATH."
    exit 1
fi

echo "t4 version: $(t4 --version 2>/dev/null || echo 'unknown')"

#--------------------------------------------------------------------
# Step 3: Configure and build
#--------------------------------------------------------------------
echo "------------------------------------------------------------"
echo "Step 3: Configuring and building Sysmon"
echo "------------------------------------------------------------"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

export PATH="$PATH:$HOME/.dotnet/tools"

cmake "$SCRIPT_DIR"

echo ""
echo "Building with $(nproc) parallel jobs..."
make -j"$(nproc)"

#--------------------------------------------------------------------
# Step 4: Verify build artifacts
#--------------------------------------------------------------------
echo "------------------------------------------------------------"
echo "Step 4: Build complete"
echo "------------------------------------------------------------"

if [[ -f "sysmon" ]]; then
    echo "SUCCESS: sysmon binary built at: $PWD/sysmon"
    ls -lh sysmon
else
    echo "ERROR: sysmon binary not found!"
    exit 1
fi

if [[ -f "sysmonUnitTests" ]]; then
    echo "SUCCESS: sysmonUnitTests built at: $PWD/sysmonUnitTests"
    ls -lh sysmonUnitTests
fi

echo ""
echo "To install and run:"
echo "  sudo ./sysmon -i"
echo "  sudo systemctl start sysmon"
echo "  tail -f /var/log/sysmon.json"
