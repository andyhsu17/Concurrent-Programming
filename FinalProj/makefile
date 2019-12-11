all: prog1 prog2 prog3 prog4 prog5
prog1: 
	g++ -fopenmp -std=c++11 msqueue.cpp -g -Wall -o msqueue
prog2:
	g++ -fopenmp -std=c++11 tstack.cpp -g -Wall -o tstack
prog3:
	g++ -fopenmp -std=c++11 sglstackqueue.cpp -g -Wall -o sglstackqueue
prog4:
	g++ -fopenmp fcstackqueue.cpp data_structures/ITest.cpp framework/cpp_framework.cpp -g -o fcstackqueue
prog5: 
	g++ -fopenmp basketqueue.cpp data_structures/ITest.cpp framework/cpp_framework.cpp -g -o basketqueue

.PHONY : clean
clean:
	rm msqueue
	rm tstack
	rm sglstackqueue
	rm fcstackqueue
	rm basketqueue
