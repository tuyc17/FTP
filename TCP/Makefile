CFLAGS = -Wall

server: server.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm *.o server
