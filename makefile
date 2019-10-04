all:
	@echo "Compiling server..."
	gcc server.c -o server
	@echo "Compiling client..."
	gcc client.c -o client
clean:
	@echo "Removing compiled files..."
	rm -rf *.o
	rm server
	rm client
