all:main
main:crawl.o socketmanager.o url.o
	g++ -o ./bin/main -L /usr/local/lib crawl.o socketmanager.o url.o -levent -levent_core
crawl.o:crawl.cpp socketmanager.hpp url.hpp
	g++ -g -c crawl.cpp
socketmanager.o: socketmanager.cpp crawl.cpp
	g++ -g -c socketmanager.cpp
url.o:url.cpp url.hpp
	g++ -g -c url.cpp

clean:
	rm -rf *.o ./bin/main