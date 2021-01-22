CC= mpicc

EXEC= bucket_mpi

$(EXEC): mains/main.c
	$(CC) mains/main.c *.c -o $(EXEC) -std=c11 -O3 -ftree-vectorize

debug: main.c
	$(CC) -DNDEBUG -g main.c -o $(EXEC) -std=c11 -O3 -ftree-vectorize

clean:
	rm -f $(EXEC) 
