all:main
main:main.o
	g++ -g ./bin/main.o -o ./bin/main 
main.o:test.cpp
	g++ -g -c test.cpp -o ./bin/main.o
