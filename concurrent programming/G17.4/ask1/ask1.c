#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <ucontext.h>
#include <stdint.h>
#include<malloc.h>
#include<memory.h>
#include<error.h>
#include <limits.h>

#define STACK_SIZE 16384

#define BUFFER_SIZE 64

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct buffer_t{
    char *array;
    int *flag;
    int size;
    FILE* fp;
};

typedef struct cor {
    ucontext_t uctx;
    ucontext_t* link;
    ucontext_t* from;
}co_t;

co_t main_co, prod_co, cons_co;

void pipe_write(char* data, int i, char temp){
	
	data[i] = temp; //write data
}

int pipe_read(char* array, int i, char *temp){
	
    *temp = array[i]; //read data
    return(1);
}

int mycoroutines_init(co_t *main){

    if(getcontext(&(main->uctx)) == -1){
        handle_error("getcontext");
        return(-1);
    }
    return(0);
}

int mycoroutines_create(co_t *co, void (body)(void *), void *arg){

    if(getcontext(&(co->uctx)) == -1){
        handle_error("getcontext");
        return(-1);
    }

    co->uctx.uc_stack.ss_sp = (char*)malloc(STACK_SIZE*sizeof(char));
    if((co->uctx.uc_stack.ss_sp) == NULL){
        return(-1);
    }

    co->uctx.uc_stack.ss_size = STACK_SIZE * sizeof(char);
    co->uctx.uc_link = co->link;
    makecontext(&(co->uctx), (void (*)(void))body, 1, arg);

    return(0);
}

int mycoroutines_switchto(co_t *co){

    if(swapcontext(co->from, &(co->uctx)) == -1){
        handle_error("swapcontext");
        return(-1);
    }

    return(0);
}

int mycoroutines_destroy(co_t *co){

    free(co->uctx.uc_stack.ss_sp);

    return(0);
}

static void producer(void* buffer){
    struct buffer_t* storage;
    int bytes_num, i;
    char temp;

    i = 0;

    cons_co.from = &prod_co.uctx;

    storage = (struct buffer_t*)buffer;
    printf("producer: started\n");

    while(1){
        if(i == BUFFER_SIZE){
            i = 0;
            printf("pipe is full, producer: swapcontext(&producer, &consumer)\n");
            mycoroutines_switchto(&cons_co);
        }
        bytes_num = fscanf(storage->fp, "%c", &temp);
        if(bytes_num == EOF){
            printf("eof\n");
            if(i == 0){
                *(storage->flag) = -2; //EOF at pos=0(beginning of the file,no bytes to read
            }
            else{
                *(storage->flag) = i - 1; //read flag bytes
            }
            printf("producer: swapcontext(&producer, &consumer), %d bytes left to read\n", *(storage->flag));
            mycoroutines_switchto(&cons_co);
            break;
        }
        pipe_write(storage->array, i, temp);
        i++;
    }

    //printf("producer: swapcontext(&producer, &consumer)\n");
    //mycoroutines_switchto(&cons_co);

    printf("producer: returning\n");
}

static void consumer(void* buffer){
    struct buffer_t* storage;
    char data;
    int i = 0; 

    storage = (struct buffer_t*)buffer;
    printf("consumer: started\n");

    while(1){
        if((*(storage->flag) == -1)){
            for(i = 0; i < BUFFER_SIZE; i++){
                pipe_read(storage->array, i, &data);
                fprintf(storage->fp, "%c", data);
            }
            i = 0;
            prod_co.from = &cons_co.uctx;
            printf("pipe is empty, consumer: swapcontext(&consumer, &producer)\n");
            mycoroutines_switchto(&prod_co);
        }
        else if((*(storage->flag)) >= 0){
            for(i = 0; i <= *(storage->flag); i++){
                pipe_read(storage->array, i, &data);
                fprintf(storage->fp, "%c", data);
            }
            break;
        }
        else{
            break;
        }
    }
    
    printf("consumer: swapcontext(&consumer, &main)\n");
    printf("consumer: returning\n");
}

int main(int argc, char *argv[]){
    struct buffer_t cons, prod;
    int flag, i;
    char buffer[BUFFER_SIZE];
    char diff_files[128];

    if(argc != 3){
        printf("Wrong arguments\n");
        return(0);
    }

    for(i = 0; i < BUFFER_SIZE; i++){
        buffer[i] = '\0';
    }

    for(i = 0; i < 128; i++){
        diff_files[i] = '\0';
    }

    cons.array = buffer;
    prod.array = buffer;

    flag = -1; //arxikopoiisi se arnitiki timh mexri na teleiwsei to input opou pairnei tin teleytaia thesi poy graftike
    cons.flag = &flag;
    prod.flag = &flag;

    prod.fp = fopen(argv[1], "r");
    if(prod.fp == NULL){
        return(0);
    }

    cons.fp = fopen(argv[2], "w+");
    if(cons.fp == NULL){
        return(0);
    }

    mycoroutines_init(&main_co);

    //prod_co.uctx.uc_link = &cons_co;
    prod_co.link = &cons_co.uctx;
    mycoroutines_create(&prod_co, &producer, &prod);

    //cons_co.uctx.uc_link = &main_co;
    cons_co.link = &main_co.uctx;    
    mycoroutines_create(&cons_co, &consumer, &cons);

    printf("main: swapcontext(&uctx_main, &uctx_producer)\n");

    prod_co.from = &main_co.uctx;
    mycoroutines_switchto(&prod_co);

    fclose(cons.fp);
    fclose(prod.fp);

    memset(diff_files,0,sizeof(diff_files));
	strcat(diff_files,"diff ");
    strcat(diff_files,argv[1]);
    strcat(diff_files," ");
    strcat(diff_files,argv[2]);
    strcat(diff_files,"\n");
    printf("%s", diff_files);
	system(diff_files);

    mycoroutines_destroy(&prod_co);
    mycoroutines_destroy(&cons_co);

    printf("main: exiting\n");
    //close file
    exit(EXIT_SUCCESS);
}
