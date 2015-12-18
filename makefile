
main: master_node.o slave_node.o main_master.o main_slave.o test.o
	g++ -o main_master main_master.o master_node.o slave_node.o \
	 -std=c++11 -O0 -g -lcaf_core -lcaf_io -pthread -lboost_serialization  
	g++ -o main_slave main_slave.o master_node.o slave_node.o \
	 -std=c++11 -O0 -g -lcaf_core -lcaf_io -pthread -lboost_serialization 
	g++ -o test gtest_main.o master_node.o slave_node.o \
 	 -std=c++11 -O0 -g -lcaf_core -lcaf_io -pthread -lboost_serialization -L./lib -lgtest

master_node.o: master_node.cpp   
	g++ master_node.cpp -std=c++11 -O0 -g -c 

slave_node.o: slave_node.cpp  
	g++ slave_node.cpp -std=c++11 -O0 -g -c  

main_master.o: main_master.cpp  
	g++ main_master.cpp -std=c++11 -O0 -g -c      

main_slave.o: main_slave.cpp  
	g++ main_slave.cpp -std=c++11 -O0 -g -c   

test.o: gtest_main.cpp
	g++ gtest_main.cpp -std=c++11 -O0 -g -c -I./lib

clean:
	rm   master_node.o slave_node.o main_master.o main_slave.o test.o