set -x
rm log.csv cpusage
g++ -std=c++11 cpu.cpp -o cpusage -O3 -s -Wall
