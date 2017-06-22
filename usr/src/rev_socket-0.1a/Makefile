## rev_socket Makefile ##

CC=gcc
CFLAGS = -Wall -Wno-strict-aliasing
STRIP_FLAG = -s
GDB_FLAGS = -g -ggdb3
ZIPDIR = rev_socket-$(shell cat rev_socket.ver)
DEPS = rev_socket.h lastflags

all: stripped

debug: CFLAGS += $(GDB_FLAGS) -DDEBUG=2 -O0

testrun: CFLAGS += -DDEBUG=1

stripped testrun: CFLAGS += -O2 $(STRIP_FLAG)

stripped debug testrun: main

checkflags:
	@[ "$(shell cat lastflags 2>/dev/null)" = "$(CFLAGS)" ] || echo "$(CFLAGS)" > lastflags

zip:
	@ln -s . $(ZIPDIR)
	@tar cvvzf $(ZIPDIR).tar.gz \
	  $(ZIPDIR)/Makefile \
	  $(ZIPDIR)/*.[ch] \
	  $(ZIPDIR)/rev_socket.ver
	@rm $(ZIPDIR)

server: rs_server.c
	$(CC) $(CFLAGS) -o rs_server rs_server.c

client : rs_client.c
	$(CC) $(CFLAGS) -o rs_client rs_client.c

main: checkflags $(DEPS) server client
	@echo -n v && ./rs_server -V | tee rev_socket.ver

clean:
	rm -vf rs_server rs_client lastflags
