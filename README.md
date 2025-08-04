You can include my_malloc.c and my_malloc.h in the project you wish to use it for. Use my_malloc() and my_free() the same way you'd use malloc() and free() from the standard library. You can also use coalesce_free_list() to clean up the free list. 

To run the test and verify that the functions work, you can use `gcc my_malloc.c malloc_test.c -o malloc_test` and then run `./malloc_test`
