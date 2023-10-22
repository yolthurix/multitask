#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "stdarg.h"

struct Task {
    int id;

    void *stack_pointer;
    void *base_pointer;
    void *instruction_pointer;

    bool setup_arguments;
    int argument_count;
    int arguments[5];
};

// hackiest queue with only 100 entries
struct Task task_queue[100];
int current_index_to_execute = 0;
int current_index_to_add = 0;

int current_id = 0;

struct Task *queue_task(void (*func)(), void *stack_pointer, void *base_pointer) {
    if (stack_pointer == NULL) {
        task_queue[current_index_to_add].stack_pointer = malloc(1000*sizeof(char)) + 500;
        task_queue[current_index_to_add].base_pointer = task_queue[current_index_to_add].stack_pointer;
        task_queue[current_index_to_add].id = current_index_to_add;
    } else {
        task_queue[current_index_to_add].stack_pointer = stack_pointer;
        task_queue[current_index_to_add].base_pointer = base_pointer;
        task_queue[current_index_to_add].id = current_id;
    }

    task_queue[current_index_to_add].instruction_pointer = func;
    task_queue[current_index_to_add].setup_arguments = false;

    return &task_queue[current_index_to_add++];
}

// floating point and excess arguments not supported
struct Task *queue_new_task(void (*func)(), int argument_count, ...) {
    va_list arg_pointer;
    va_start(arg_pointer, argument_count);

    struct Task *task = queue_task(func, NULL, NULL);

    int i;

    for (i = 0; i < argument_count; i++) {
        task->arguments[i] = va_arg(arg_pointer, int);
    }

    va_end(arg_pointer);

    task->argument_count = i;
    task->setup_arguments = true;

    return task;
}

#define current_task (&task_queue[current_index_to_execute-1])

void handle_function_return() {
    current_index_to_execute++;
    
    if (current_task->setup_arguments) {
        for (int i = 0; i <= current_task->argument_count; i++) {
            switch (i) { // add more
                case 0:
                    asm ("mov edi, %0" :: "r" (current_task->arguments[i]));
                    break;
                case 1:
                    asm ("mov esi, %0" :: "r" (current_task->arguments[i]));
                    break;
                case 2:
                    asm ("mov edx, %0" :: "r" (current_task->arguments[i]));
                    break;
            }
        }

        current_task->setup_arguments = false;
    }

    asm ("mov rsp, %0;"
        :
        : "r" (current_task->stack_pointer));

    asm ("mov rbp, %0;"
        :
        : "r" (current_task->base_pointer));

    asm ("mov [rbp], %0;"
        :
        : "r" (handle_function_return));

    asm ("jmp %0;"
        :
        : "r" (current_task->instruction_pointer));
}

__always_inline
inline void yield() {
    void *return_address = &&label;
    void *original_base_pointer;
    void *original_stack_pointer;

    asm ("mov %0, rbp;"
            : "=r" (original_base_pointer));

    asm ("mov %0, rsp;"
            : "=r" (original_stack_pointer));

    queue_task(return_address, original_stack_pointer, original_base_pointer);

    // setupping to go to next task

    current_index_to_execute++;
    
    if (current_task->setup_arguments) {
        for (int i = 0; i <= current_task->argument_count; i++) {
            switch (i) { // add more
                case 0:
                    asm ("mov edi, %0" :: "r" (current_task->arguments[i]));
                    break;
                case 1:
                    asm ("mov esi, %0" :: "r" (current_task->arguments[i]));
                    break;
                case 2:
                    asm ("mov edx, %0" :: "r" (current_task->arguments[i]));
                    break;
            }
        }

        current_task->setup_arguments = false;
    }

    asm ("mov rsp, %0;"
        :
        : "r" (current_task->stack_pointer));

    asm ("mov rbp, %0;"
        :
        : "r" (current_task->base_pointer));

    asm ("mov [rbp], %0;"
        :
        : "r" (handle_function_return));

    asm ("jmp %0;"
        :
        : "r" (current_task->instruction_pointer));
    
    label: ;
}

void test_func(int x, int y) {
    int z = x + y;
    return;
}

int main() {
    queue_new_task(test_func, 2, 3, 4);
    yield();

    return 0;
}
