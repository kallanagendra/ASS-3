CC=cc
CFLAGS=-Werror -g

default: oss palin

oss: oss.c
	$(CC) $(CFLAGS) oss.c -o oss

palin: palin.c
	$(CC) $(CFLAGS) palin.c -o palin

clean:
	rm oss palin palin.out nopalin.out output.txt
