all:main
main:crawl.o socketmanager.o Message_Queue.o bloomfilter.o
	g++ -std=c++11 -o crawl crawl.o socketmanager.o Message_Queue.o bloomfilter.o -levent -lpthread
crawl.o:crawl.cpp socketmanager.hpp bloomfilter.hpp Message_Queue.cpp
	gcc -std=c++11 -g -c crawl.cpp
socketmanager.o: socketmanager.cpp bloomfilter.hpp Message_Queue.cpp ThreadPool.h
	gcc -std=c++11 -g -c socketmanager.cpp
Message_Queue.o: Message_Queue.cpp
	gcc -g -c Message_Queue.cpp
bloomfilter.o:bloomfilter.cpp bloomfilter.hpp
	gcc -std=c++11 -g -c bloomfilter.cpp

clean:
	rm -rf *.o ./bin/main
