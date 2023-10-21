#include "stdio.h"
#include "stdlib.h"

struct Task {
    void *stack_pointer;
    void *base_pointer;
    void *instruction_pointer;
};

// hackiest queue with only 100 entries
struct Task task_queue[100];
int current_index_to_execute = 0;
int current_index_to_add = 0;

void queue_task(void (*func)(), void *stack_pointer, void *base_pointer) {
    if (stack_pointer == NULL) {
        task_queue[current_index_to_add].stack_pointer = malloc(100*sizeof(char)) + 80;
        task_queue[current_index_to_add].base_pointer = task_queue[current_index_to_add].stack_pointer;
    } else {
        task_queue[current_index_to_add].stack_pointer = stack_pointer;
        task_queue[current_index_to_add].base_pointer = base_pointer;
    }

    task_queue[current_index_to_add].instruction_pointer = func;

    current_index_to_add++;
}

void handle_function_return() {
    asm ("mov rsp, %0;"
        :
        : "r" (task_queue[current_index_to_execute].stack_pointer));

    asm ("mov rbp, %0;"
        :
        : "r" (task_queue[current_index_to_execute].base_pointer));

    asm ("mov [rbp], %0;"
        :
        : "r" (handle_function_return));

    asm ("jmp %0;"
        :
        : "r" (task_queue[current_index_to_execute++].instruction_pointer));
}

//havainto: jos funktio sisältää funktio callin, funktio laittaa stack pointterin alaspäin funktio callia varten

void yield() {
    void *return_address;
    void *original_base_pointer;
    void *original_stack_pointer;

    asm ("mov %0, [rbp];"
            : "=r" (original_base_pointer));

    asm ("mov %0, [rbp+8];"
            : "=r" (return_address));

    asm ("mov %0, [rbp+16];"
            : "=r" (original_stack_pointer));

    queue_task(return_address, original_stack_pointer, original_base_pointer);

    // setupping to go to next task
    asm ("leave;");

    asm ("mov rsp, %0;"
        :
        : "r" (task_queue[current_index_to_execute].stack_pointer));

    asm ("mov rbp, %0;"
        :
        : "r" (task_queue[current_index_to_execute].base_pointer));

    asm ("mov [rbp], %0;"
        :
        : "r" (handle_function_return));

    asm ("jmp %0;"
        :
        : "r" (task_queue[current_index_to_execute++].instruction_pointer));
}

void test_func() {
    int x = 1 + 2;
    printf("test\n");
    return;
}

int main() {
    queue_task(test_func, NULL, NULL);
    yield();

    char *stack = malloc(100*sizeof(char));
    asm ("mov rsp, %0;"
            :
            : "r" (stack+100)
            :);
    printf("%p\n", stack);
    printf("%p\n", &stack);

    test_func();
    return 0;
}
