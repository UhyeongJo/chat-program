CC = gcc

all: server.out c1-term.out c1-disp.out c2-term.out c2-disp.out c3-term.out c3-disp.out c4-term.out c4-disp.out

server.out: server.c
	$(CC) -o server.out $^ -lpthread

c1-term.out: c1-term.c
	$(CC) -o c1-term.out $^
c1-disp.out: c1-disp.c
	$(CC) -o c1-disp.out $^

c2-term.out: c2-term.c
	$(CC) -o c2-term.out $^
c2-disp.out: c2-disp.c
	$(CC) -o c2-disp.out $^

c3-term.out: c3-term.c
	$(CC) -o c3-term.out $^
c3-disp.out: c3-disp.c
	$(CC) -o c3-disp.out $^

c4-term.out: c4-term.c
	$(CC) -o c4-term.out $^
c4-disp.out: c4-disp.c
	$(CC) -o c4-disp.out $^

clean1:
	rm server.out c1-term.out c1-disp.out c2-term.out c2-disp.out c3-term.out c3-disp.out c4-term.out c4-disp.out
clean2:
	rm c1 c2 c3 c4

