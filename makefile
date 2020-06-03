all: build_c build_o clean

build_o: 
	gcc -o client client.o
	gcc -o server server.o

build_c: 
	gcc -c src/client.c 
	gcc -c src/server.c 

clean:
	rm *.o

run_client:
	./client $(name)

run_server:
	./server $(name)