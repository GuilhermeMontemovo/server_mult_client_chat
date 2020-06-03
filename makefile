.SILENT:
all: build_c build_o clean

build_o: 
	gcc -o client client.o -l pthread
	gcc -o server server.o -l pthread

build_c: 
	gcc -c src/client.c
	gcc -c src/server.c

clean:
	rm *.o -f

run_client:
	./client $(name)

run_server:
	./server