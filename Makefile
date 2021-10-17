compFlags := -g
port := 1245
ip := 127.0.0.1
debug := # valgrind

# pentru debugging si impachetare
build:
	make server
	make subscriber
srv:
	$(debug) ./server $(port)
c1:
	$(debug) ./subscriber C1 $(ip) $(port)
c2:
	$(debug) ./subscriber C2 $(ip) $(port)
pack:
	zip PCHW2_TomaStefanMadalin_323CC.zip README.md Makefile *.h *.cpp

# pentru testare pe checker
server:
	g++ server.cpp message.cpp $(compFlags) -o server
subscriber:
	g++ subscriber.cpp message.cpp $(compFlags) -o subscriber
clean:
	rm -rf server subscriber
