CXX=g++
CXXFLAGS=--std=c++11 -O3 -lpthread

tiny-space: tiny-space.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS)
