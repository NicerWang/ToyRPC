CC=gcc
RPC_SYSTEM=rpc.o
LDFLAGS=-std=c11

all: $(RPC_SYSTEM)

$(RPC_SYSTEM): rpc.c rpc.h protocol.h
	$(CC) -Wall -g -c -o $@ $< $(LDFLAGS)

RPC_SYSTEM_A=rpc.a

all: $(RPC_SYSTEM_A)
$(RPC_SYSTEM_A): rpc.o
	ar rcs $(RPC_SYSTEM_A) $(RPC_SYSTEM)

clean:
	rm -f *.o rpc
	rm -f *.a rpc
