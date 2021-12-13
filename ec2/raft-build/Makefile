
default:
	mkdir -p build
	g++ -O3 -std=c++11 -w -I raft/ -o build/server raft/raft.cpp common-src/common.cpp common-src/kvstore.cpp common-src/server.cpp -lpthread

.PHONY: default
