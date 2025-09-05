# Since my name is Makefile-2 but not the default Makefile,
# to use me you have to specify: make -f Makefile-2

moab_server : server.o game_logic.o client_handler.o
	gcc -g -pthread $^ -o $@

%.o : %.c server.h
	gcc -g -c $<

server.o : server.h
game_logic.o : server.h
client_handler.o : server.h

clean :
	rm -f *.o moab_server
.PHONY: clean 