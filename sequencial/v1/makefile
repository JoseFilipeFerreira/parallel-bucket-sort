CC= gcc

RANDOM= generate
RANDOM_FILE= random.txt
EXEC= bucket_seq

$(EXEC): main.c
	$(CC) main.c -o $(EXEC) -std=c11 -O3 -ftree-vectorize

debug: main.c
	$(CC) -DNDEBUG -g main.c -o $(EXEC) -std=c11 -O3 -ftree-vectorize


grind: $(EXEC) $(RANDOM_FILE)
	valgrind --leak-check=full --show-reachable=no --show-leak-kinds=all ./$(EXEC) $(RANDOM_FILE)

$(RANDOM_FILE): $(RANDOM)
	./$(RANDOM) -100 100 1000 $(RANDOM_FILE)


clean:
	rm -f $(EXEC) $(RANDOM_FILE)
