all: wc ls
ls:
	gcc -c ls.c -o ls.o
	gcc ls.o -o mls
wc:
	gcc -c wc.c -o wc.o
	gcc wc.o -o mwc