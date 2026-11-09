CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic

all: twmailer-client twmailer-server

twmailer-client: client/client.cpp client/client.h
	$(CXX) $(CXXFLAGS) client/client.cpp -o twmailer-client

twmailer-server: server/server.cpp server/server.h
	$(CXX) $(CXXFLAGS) server/server.cpp -o twmailer-server

clean:
	rm -f twmailer-client twmailer-server
