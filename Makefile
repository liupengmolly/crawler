all:main
main:crawl.o socketmanager.o Message_Queue.o bloomfilter.o thpool.o
	g++ -std=c++11 -o crawl crawl.o socketmanager.o Message_Queue.o bloomfilter.o thpool.o -levent -lpthread
crawl.o:crawl.cpp socketmanager.hpp bloomfilter.hpp Message_Queue.cpp
	g++ -std=c++11 -g -c crawl.cpp
socketmanager.o: socketmanager.cpp bloomfilter.hpp Message_Queue.cpp thpool.h
	g++ -std=c++11 -g -c socketmanager.cpp
Message_Queue.o: Message_Queue.cpp
	g++ -g -c Message_Queue.cpp
bloomfilter.o:bloomfilter.cpp bloomfilter.hpp
	g++ -std=c++11 -g -c bloomfilter.cpp
thpool.o:thpool.c thpool.h
	gcc -g -c thpool.c

clean:
	rm -rf *.o ./bin/main
