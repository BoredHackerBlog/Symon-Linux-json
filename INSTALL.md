# Sysmon-Linux JSON Output

This repository contains a complete, standalone copy of [Sysmon-Linux](https://github.com/microsoft/Sysmon-Linux)
modified to output event logs in JSON format to `/var/log/sysmon.json` instead of XML to syslog.

## What Was Changed

| File | Change |
|------|--------|
| `outputxml.cpp` | Replaced XML-over-syslog output with JSON output to `/var/log/sysmon.json` |
| `sysmonforlinux.c` | Removed syslog() integration (`openlog`/`closelog`/`syslogHelper` made no-op) |
| `CMakeLists.txt` | Use system OpenSSL instead of building from source |

### Quick Build (Recommended)

The simplest way to build is using the automated script:

```bash
./build-sysmon.sh
```

This handles dependency installation, Microsoft repository setup,
dotnet-t4 installation, cmake configuration, and compilation.

Specify a custom build directory:

```bash
./build-sysmon.sh /tmp/sysmon-build
```

---

## Build on Ubuntu

### 1. Install build dependencies

```bash
sudo apt-get update
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
```

### 2. Add Microsoft package repository

```bash
wget -q https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
sudo apt-get update
```

### 3. Install dotnet-t4 tool (required for code generation during build)

```bash
dotnet tool install --global dotnet-t4 --version 2.3.1
export PATH="$PATH:$HOME/.dotnet/tools"
```

### 4. Build

```bash
mkdir build
cd build
export PATH="$PATH:$HOME/.dotnet/tools"
cmake ..
make -j$(nproc)
```

Build artifacts:
- `sysmon` — the Sysmon binary
- `sysmonUnitTests` — unit test binary

## Install and Run

```bash
# Install as a system service
sudo ./sysmon -i

# Start the service
sudo systemctl start sysmon

# Check status
sudo systemctl status sysmon
```

## Verify JSON Output

Events are written to `/var/log/sysmon.json` (one JSON object per line):

```bash
ls -la /var/log/sysmon.json
head -5 /var/log/sysmon.json
```

Validate that all lines are valid JSON:

```bash
python3 -c "
import json
with open('/var/log/sysmon.json') as f:
    count = 0
    for i, line in enumerate(f, 1):
        line = line.strip()
        if not line:
            continue
        try:
            json.loads(line)
            count += 1
        except json.JSONDecodeError as e:
            print(f'INVALID line {i}: {e}')
            break
    print(f'Validated {count} JSON event lines')
"
```

Each JSON event contains: `EventID`, `Version`, `Level`, `Task`, `Opcode`, `Keywords`,
`TimeCreated`, `EventRecordID`, `ProcessID`, `ThreadID`, `Channel`, `Computer`, `UserId`,
and `EventData` (with named field-value pairs).

### Example output

```json
{"EventID":1,"Version":3,"Level":4,"Task":1,"Opcode":0,"Keywords":"0x8000000000000000","TimeCreated":"2026-11-08T18:13:08.835502000Z","EventRecordID":0,"ProcessID":1234,"ThreadID":1234,"Channel":"Linux-Sysmon/Operational","Computer":"hostname","UserId":0,"EventData":{"UtcTime":"...","Configuration":"..."}}
```

## Stop and Uninstall

```bash
sudo systemctl stop sysmon
sudo ./sysmon -u force
```

## Repository Contents

This is a complete, self-contained copy of Sysmon-Linux with all source files,
the `sysmonCommon` submodule contents, `pugixml` library, and build scripts.
No external cloning or manual patching is required.
