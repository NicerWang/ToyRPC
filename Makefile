CC=gcc
RPC_SYSTEM=rpc.o
LDFLAGS=-std=c99

all: $(RPC_SYSTEM)

$(RPC_SYSTEM): rpc.c rpc.h protocol.h
	$(CC) -Wall -g -c -o $@ $< $(LDFLAGS)

RPC_SYSTEM_A=rpc.a

all: $(RPC_SYSTEM_A)
$(RPC_SYSTEM_A): rpc.o
	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

test:
	$(CC) -g -o server server.c rpc.a $(LDFLAGS)
	$(CC) -g -o client client.c rpc.a $(LDFLAGS)

clean:
	rm -f *.o rpc
	rm -f *.a rpc
	rm -f server
	rm -f client
