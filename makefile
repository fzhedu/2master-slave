
main: master_node.o slave_node.o main_master.o main_slave.o
	g++ -o main_master main_master.o master_node.o slave_node.o \
	 -std=c++11 -O0 -g -lcaf_core -lcaf_io -pthread
	g++ -o main_slave main_slave.o master_node.o slave_node.o \
	 -std=c++11 -O0 -g -lcaf_core -lcaf_io -pthread
	 

master_node.o: master_node.cpp based_node.h master_node.h 
	g++ master_node.cpp -std=c++11 -O0 -g -c -lcaf_core -lcaf_io

slave_node.o: slave_node.cpp based_node.h slave_node.h
	g++ slave_node.cpp -std=c++11 -O0 -g -c -lcaf_core -lcaf_io

main_master.o: main_master.cpp master_node.h slave_node.h
	g++ main_master.cpp -std=c++11 -O0 -g -c -lcaf_core -lcaf_io

main_slave.o: main_slave.cpp master_node.h slave_node.h
	g++ main_slave.cpp -std=c++11 -O0 -g -c -lcaf_core -lcaf_io


clean:
	rm   master_node.o slave_node.o main_master.o main_slave.o