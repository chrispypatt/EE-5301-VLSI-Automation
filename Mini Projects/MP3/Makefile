all: placement

placement: placement.o  
	g++  placement.o parser.o helpers.o graph.o -o placement -std=c++11
	
placement.o: placement.cpp placement.h parser.o graph.o
	g++ -c placement.cpp -std=c++11

parser.o: parser.cpp parser.h helpers.o
	g++ -c parser.cpp -std=c++11

graph.o: graph.cpp graph.h
	g++ -c graph.cpp -std=c++11

helpers.o: helpers.cpp helpers.h
	g++ -c helpers.cpp -std=c++11

clean:
	rm placement parser.o helpers.o placement.o graph.o