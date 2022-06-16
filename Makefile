F=fs

main: $F/main.o $F/init.o $F/fs.o $F/func.o
	gcc $F/main.o $F/init.o $F/fs.o $F/func.o -o main
	./main

main.o: $F/fs.h $F/defs.h $F/main.c
	gcc -c $F/main.c $F/main.o

init.o: $F/fs.h $F/defs.h $F/init.c
	gcc -c $F/init.c $F/init.o

fs.o: $F/fs.h $F/fs.c
	gcc -c $F/fs.c $F/fs.o

func.o: $F/func.c
	gcc -c $F/func.c $F/func.o

clean:
	@echo "cleaning project"
	-rm $F/*.o main img
	@echo "clean completed"

.PHONY: clean
