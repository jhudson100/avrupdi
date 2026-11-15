all: jsontest

jsontest: jsontest.cpp ../Json.h ../Json.cpp
	$(CXX) -g -Wall -I.. jsontest.cpp ../Json.cpp -o $@
