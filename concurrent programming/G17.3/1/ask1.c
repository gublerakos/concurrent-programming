#define _GNU_SOURCE
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/stat.h>

enum boolean{ false, true };

pthread_cond_t ready, working, finish, job, last_thread;
pthread_mutex_t mtx;

struct info_t{
	int* array;
	enum boolean* flag;
};

// Struct with 3 variables, value for the input number, status to know if a thread is available or not and thread_id.
struct thread_t{
    pthread_t thread_id;
    int number;
    enum boolean term;
    int status;
    int *counter;
	struct info_t* info;
    int *available;
	int *working;
	int *flag;
	int *exited;
	int nthreads;
};

// Function that tests if a number is prime or not.
int primetest(int number){
    int i;

    for(i = number - 1; i > 1; i--){
        if((number%i) == 0){
            return(0);
        }
    }
    return(1);
}

void *worker_func(void *ptr1){
    struct thread_t* ptr;
    void* retval = 0;
	int result, ret_val, i;

    ptr =(struct thread_t *)ptr1;
	
    while(1){
		
        ret_val = pthread_mutex_lock(&mtx);
		if(ret_val){
			printf("mutex lock error\n");
		}
		*(ptr->working) = *(ptr->working) - 1;

		printf("working = %d\n", *(ptr->working));
		if((*(ptr->counter) == 0) && (ptr->term == true) && (*(ptr->working) == (-(ptr->nthreads)))){
			ret_val = pthread_cond_signal(&finish);
			if(ret_val){
				printf("cond_signal error\n");
			}
			printf("notifying main that all threads are available\n");
			break;
		}

		if((*(ptr->working)) < 0){
			ret_val = pthread_cond_wait(&(job), &mtx);
			if(ret_val){
				printf("cond_wait error\n");
			}
		}

		if((*(ptr->counter) == 0) && (ptr->term == true)){
			break;
		}
		
		for(i = 0; i < (ptr->nthreads); i++){
			if(ptr->info->flag[i] == 1){
				ptr->number = ptr->info->array[i];
				ptr->info->flag[i] = 0;
				break;
			}
		}
		
		ret_val = pthread_mutex_unlock(&mtx);
		if(ret_val){
			printf("mutex unlock error\n");
		}
		
        result = primetest(ptr->number);
		if(result == 1){
			printf("-----------------------%d: prime!\n", ptr->number);
		}
		if(result == 0){
			printf("-----------------------%d: not prime!\n", ptr->number);
		}
		
		ret_val = pthread_mutex_lock(&mtx);
		if(ret_val){
			printf("mutex lock error\n");
		}
		
		*(ptr->counter) = *(ptr->counter) - 1;
		*(ptr->available) = *(ptr->available) + 1;
		ptr->status = 1;
		
		if(*(ptr->available) <= 0){
			ret_val = pthread_cond_signal(&ready);
			if(ret_val){
				printf("signal error\n");
			}
		}
		
		ret_val = pthread_mutex_unlock(&mtx);
		if(ret_val){
			printf("mutex unlock error\n");
		}
        pthread_yield();
    }
    
    printf("Thread %lud is exiting\n", pthread_self());
    
    *(ptr->exited) = *(ptr->exited) + 1;
	printf("exited = %d\n", *(ptr->exited));
	if(*(ptr->exited) == (ptr->nthreads)){
		ret_val = pthread_cond_signal(&last_thread);
		if(ret_val){
			printf("signal error\n");
		}
	}
	
	ret_val = pthread_mutex_unlock(&mtx);
	if(ret_val){
		printf("mutex unlock error\n");
	}

	pthread_exit(retval);
}

int main(int argc, char* argv[]){
    int i, number, res, counter, ret_val, flag, available;
	int working, exited;
	int nthreads;
	struct info_t* info;
    int all_numbers = 0;
	
	if(argv[1] == NULL){
		return(0);
	}
	//getting the size from args
    nthreads = atoi(argv[1]);
	
	info = (struct info_t*)malloc(sizeof(struct info_t));
    if(info == NULL){
        printf("malloc error\n");
    }
	info->array = (int*)malloc(nthreads*sizeof(int));
    if(info->array == NULL){
        printf("malloc error\n");
    }
	info->flag = (enum boolean*)malloc(nthreads*sizeof(enum boolean));
    if(info->flag == NULL){
        printf("malloc error\n");
    }

	for(i = 0; i < nthreads; i++){
		info->array[i] = 0;
		info->flag[i] = 0;
	}
	
    printf("threads:%d\n", nthreads);
    counter = 0;
	flag = 0;
	available = nthreads;
	working = 0;
	exited = 0;

	pthread_mutex_init(&mtx, NULL);
	pthread_cond_init(&ready, NULL);
	pthread_cond_init(&job, NULL);
	pthread_cond_init(&finish, NULL);
	pthread_cond_init(&last_thread, NULL);

    struct thread_t worker[nthreads];
	
    // Creating workers.
    for(i = 0; i < nthreads; i++){
        worker[i].counter = &counter;
       	worker[i].nthreads = nthreads;
        worker[i].status = 1;
		worker[i].flag = &flag;
        worker[i].term = false;
	worker[i].available = &available;
	worker[i].working = &working;
	worker[i].exited = &exited;
	worker[i].info = info;
		
        res = pthread_create(&(worker[i].thread_id), NULL, &worker_func, (void*)&worker[i]);
	printf("created thread id: %lud\n", worker[i].thread_id);
        if(res){
            	perror("Error");
        }
    }
    
    //taking numbers with scanf.
	while(1){
		scanf("%d", &number);
		if(number <= 1){
			ret_val = pthread_mutex_lock(&mtx);
			if(ret_val){
			 	printf("mutex lock error\n");
			}

			for(i = 0; i < nthreads; i++){
				worker[i].term = true;
			}	
			if(working == -nthreads){
				for(i = 0; i < nthreads; i++){
					ret_val = pthread_cond_signal(&job);
					if(ret_val){
						printf("signal error\n");
					}
				}
				break;
			}
			else{
				ret_val = pthread_cond_wait(&finish, &mtx);
				if(ret_val){
					printf("wait error\n");
				}
				printf("main alive\n");
				for(i = 0; i < nthreads - 1; i++){
					printf("main signals job\n");
					ret_val = pthread_cond_signal(&job);
					if(ret_val){
						printf("signal error\n");
					}
				}
				break;
			}
		}
		//conditions to exit

		ret_val = pthread_mutex_lock(&mtx);
		if(ret_val){
			printf("mutex lock error\n");
		}
		
        counter++;
        all_numbers++;
		available--;
		//assign numbers to available workers.
		
		if(available < 0){
			printf("no one available, main going to sleep\n");
			ret_val = pthread_cond_wait(&ready, &mtx);
			if(ret_val){	
				printf("cond_wait error\n");
			}
		}
		
		
		for(i = 0; i < nthreads; i++){
			if(info->flag[i] == 0){
				info->array[i] = number;
				info->flag[i] = 1;
				break;
			}
		}
		working++;
		
		if(working <= 0){
			ret_val = pthread_cond_signal(&(job));
			if(ret_val){
				printf("cond_signal error\n");
			}
		}

		ret_val = pthread_mutex_unlock(&mtx);
		if(ret_val){
			printf("unlock error\n");
		}
		
		pthread_yield();
	}

	if(exited < nthreads){
		ret_val = pthread_cond_wait(&last_thread, &mtx);
		if(ret_val){
			printf("wait error\n");
		}
	}

	ret_val = pthread_mutex_unlock(&mtx);
	if(ret_val){
		printf("unlock error\n");
	}

	free(info->array);
	free(info->flag);
	free(info);
	
    printf("all numbers = %d\n", all_numbers);
	printf("all exited, main is exiting too\n");
	
    return(0);
}
