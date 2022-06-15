all: main

F=fs
A=apps

main: main.o init.o shm.o fs.o func.o
	gcc main.o init.o fs.o shm.o func.o -o main
	./main

main.o: $F/fs.h $F/defs.h $F/main.c
	gcc -c $F/main.c -g

init.o: $F/fs.h $F/defs.h $F/init.c
	gcc -c $F/init.c -g

fs.o: $F/fs.h $F/fs.c
	gcc -c $F/fs.c -g

shm.o: $F/shm.c
	gcc -c $F/shm.c -g

func.o: $F/func.c
	gcc -c $F/func.c -g

clean:
	@echo "cleaning project"
	-rm *.o main img
	@echo "clean completed"

.PHONY: clean
