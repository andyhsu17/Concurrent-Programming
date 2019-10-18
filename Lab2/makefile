all: prog1 prog2
prog1: 
	g++ -std=c++11 mysort.cpp locksbarriers.cpp -pthread -O3 -g -o mysort
prog2:
	g++ -std=c++11 counter.cpp locksbarriers.cpp -pthread -O3 -g -o counter

.PHONY : clean
clean:
	rm mysort
	rm counter