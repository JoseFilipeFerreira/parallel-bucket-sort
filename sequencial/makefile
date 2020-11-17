CC= gcc

EXEC= main

$(EXEC): main.c
	$(CC) main.c -o $(EXEC)

debug: main.c
	$(CC) -DNDEBUG -g main.c -o $(EXEC)

grind: $(EXEC)
	valgrind --leak-check=full --show-reachable=no --show-leak-kinds=all ./$(EXEC)

clean: $(EXEC)
	rm $(EXEC)
