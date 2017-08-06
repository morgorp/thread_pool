test : test.c thread_pool.c thread_pool.h
	gcc -o $@ -lpthread test.c thread_pool.c

run :
	./test
.PHONY : run

clean :
	-rm -f test
.PHONY : clean

