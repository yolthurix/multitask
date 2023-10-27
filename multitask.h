#ifndef MULTITASK_H
#define MULTITASK_H

#include "stdlib.h"
#include "stdarg.h"
#include "stdbool.h"

#define INITIAL_MAX_TASKS 100

struct Task {
    void *instruction_pointer;
    void *stack_pointer;
    void *base_pointer;

    bool setup_args;
    int arg_count;
    long args[6];
};

void multitask_queue_task(struct Task task);
struct Task *multitask_get_next_task();
struct Task multitask_create_task_struct(void *ins_ptr, void *base_ptr, void *stack_ptr, int arg_count, long args[6]);
struct Task multitask_create_task(void *ins_ptr, bool allocate_new_stack, int arg_count, ...);
void multitask_handle_function_return();
void multitask_yield(bool queue_continuation);

int max_tasks = INITIAL_MAX_TASKS;
int task_count = 0;

struct Task *next_free_task = NULL;
struct Task *next_executable_task = NULL;
struct Task *tasks = NULL;

void multitask_queue_task(struct Task task) {
    if (tasks == NULL) {
        tasks = calloc(max_tasks, sizeof(struct Task));
        next_free_task = tasks;
        next_executable_task = tasks;
    }

    if (next_free_task == tasks + max_tasks) {
        next_free_task = tasks;
    }
    
    *next_free_task++ = task;
}

struct Task *multitask_get_next_task() {
    if (next_executable_task == tasks + max_tasks) {
        next_executable_task = tasks;
    }

    return next_executable_task++;
}

struct Task multitask_create_task_struct(void *ins_ptr, void *base_ptr, void *stack_ptr, int arg_count, long args[6]) {
    struct Task task;

    task.instruction_pointer = ins_ptr;
    task.stack_pointer = stack_ptr;
    task.base_pointer = base_ptr;
    task.setup_args = arg_count;
    task.arg_count = arg_count;

    for (int i = 0; i < arg_count; i++) {
        task.args[i] = args[i];
    }

    return task; 
}

struct Task multitask_create_task(void *ins_ptr, bool allocate_new_stack, int arg_count, ...) {
    void *stack_ptr, *base_ptr;
    long args[6];

    if (allocate_new_stack) {
        stack_ptr = malloc(1000*sizeof(char)) + 500;
        base_ptr = stack_ptr;

        va_list va_ptr;
        va_start(va_ptr, arg_count);
        
        for (int i = 0; i < arg_count; i++) {
            args[i] = va_arg(va_ptr, long);
        }

        va_end(va_ptr);

        asm ("mov [%1], %0;" :: "r" (multitask_handle_function_return), "r" (base_ptr));
    } else {
        asm ("mov rax, rbp; add rax, 0x10; mov %0, rax;" : "=r" (stack_ptr));
        asm ("mov %0, [rbp];" : "=r" (base_ptr));
    }

    return multitask_create_task_struct(ins_ptr, base_ptr, stack_ptr, arg_count, args);
}

void multitask_handle_function_return() {
    register struct Task *task = multitask_get_next_task();
    
    if (task->setup_args) {
        for (int i = 0; i <= task->arg_count; i++) {
            switch (i) { // add more
                case 0:
                    asm ("mov rdi, %0" :: "r" (task->args[i]));
                    break;
                case 1:
                    asm ("mov rsi, %0" :: "r" (task->args[i]));
                    break;
                case 2:
                    asm ("mov rdx, %0" :: "r" (task->args[i]));
                    break;
            }
        }

        task->setup_args = false;
    }

    asm ("mov rsp, %0;" :: "r" (task->stack_pointer));
    asm ("mov rbp, %0;" :: "r" (task->base_pointer));
    asm ("jmp %0;" :: "r" (task->instruction_pointer));
}

void multitask_yield(bool queue_continuation) {
    if (queue_continuation) {
        void *stack_ptr, *base_ptr, *return_address;

        asm ("mov rax, rbp; add rax, 0x10; mov %0, rax;" : "=r" (stack_ptr));
        asm ("mov %0, [rbp];" : "=r" (base_ptr));
        asm ("mov %0, [rbp + 8];" : "=r" (return_address));

        struct Task task = multitask_create_task_struct(return_address, base_ptr, stack_ptr, 0, NULL);

        multitask_queue_task(task);
    }

    // this is going to segfault if queue_continuation is false and tasks are not initialized
    // or if tasks are initialized but no new tasks are queued this is going to result in undefined behaviour
    register struct Task *task = multitask_get_next_task(); 
    
    if (task->setup_args) {
        for (int i = 0; i <= task->arg_count; i++) {
            switch (i) { // add more
                case 0:
                    asm ("mov rdi, %0" :: "r" (task->args[i]));
                    break;
                case 1:
                    asm ("mov rsi, %0" :: "r" (task->args[i]));
                    break;
                case 2:
                    asm ("mov rdx, %0" :: "r" (task->args[i]));
                    break;
            }
        }

        task->setup_args = false;
    }

    asm ("mov rsp, %0;" :: "r" (task->stack_pointer));
    asm ("mov rbp, %0;" :: "r" (task->base_pointer));
    asm ("jmp %0;" :: "r" (task->instruction_pointer));
    
    label: ;
}

#endif