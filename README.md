# Symon-Linux-json
sysmon for linux that outputs json 

# AI Warning
The code was cloned from Microsoft Sysmon-Linux repo and rewritten by AI to support json output. 

Model Provider: Nvidia
Model: nemotron-3.5-super-vl-120b-a12b
Agent: Cline Bot

Why AI? usually i just manually write code because i enjoy it. in this case, i dont know enough to rewrite this code. i wanted to spend my time on my project which i enjoy doing rather than figuring out json support. 

This project has not been tested for stability at all. it should be used in a lab!! 

This repo likely wont be updated!!

# Usage
I tested it on ubuntu 26.04.

- Clone the repo
- make build-sysmon.sh executable `chmod +x build-sysmon.sh`
- run build-sysmon.sh with an output path `./build-sysmon.sh ../build`
- cd into ../build or whatever and install sysmon like normal with your rule `./sysmon -i whateverconfig`
- logs are in /var/log/sysmon.json

Again, this is AI generated so do not expect stability. it should be good enough for testing in a lab.

