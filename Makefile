CXX=g++
CXXFLAGS=--std=c++11 -O3 -lpthread

SRC=$(wildcard src/*.cpp)

tinyspace: $(SRC)
	$(CXX) -o $@ $^ $(CXXFLAGS)
