
default:
	mkdir -p build
	g++ -O3 -std=c++11 -w -DMULTIPAXOS -I multi-paxos/ -o build/server multi-paxos/multi_paxos_system.cpp common-src/common.cpp common-src/kvstore.cpp common-src/server.cpp -lpthread

.PHONY: default
