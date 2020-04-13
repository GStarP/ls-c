all: clean
	gcc -c ls.c -o ls.o
	gcc ls.o -o my_ls

clean:
	sudo rm ls.o my_ls