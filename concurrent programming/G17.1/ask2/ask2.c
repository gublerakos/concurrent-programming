#define _GNU_SOURCE
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

// Struct with 3 variables, value for the input number, status to know if a thread is available or not and thread_id.
struct thread_t{
    pthread_t thread_id;
    int status;
    int value;
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
	int result;

    ptr =(struct thread_t *)ptr1;
    
    while(1){
        while(1){
            if(((ptr->status) == 1) || ((ptr->status) == -1)){ //if a worker is busy or is notified by main to terminate --> break, else yield.
                break;
            }
            pthread_yield();
        }
        if((ptr->status) == -1){
            break;
        }
        
        //checking if worker's status is 1 to call primetest.
        if((ptr->status) == 1){
            result = primetest(ptr->value);
			if(result == 1){
				printf("%d: prime!\n", ptr->value);
			}
			if(result == 0){
				printf("%d: not prime!\n", ptr->value);
			}
			ptr->status = 0;
        }
        pthread_yield();
    }

    //changing status to -2 to notify main a worker is actually going to terminate.
    (ptr->status) = -2;
   
    pthread_exit(retval);
}

int main(int argc, char* argv[]){
    int i, nthreads, number, j, res, flag;

	if(argv[1] == NULL){
		return(0);
	}
	// getting the size from args
    nthreads = atoi(argv[1]);
    printf("threads:%d\n", nthreads);

    struct thread_t worker[nthreads];
	
    // Creating workers.
    for(i = 0; i < nthreads; i++){
		worker[i].status = 0;
        res = pthread_create(&(worker[i].thread_id), NULL, &worker_func, (void*)&worker[i]);
		printf("created thread id: %lud\n", worker[i].thread_id);
        if(res){
            perror("Error");
        }
    }
    
    j = 0;
    
    //taking numbers with scanf.
	do{
		scanf("%d", &number);
		if(number <= 1){
			printf("exiting\n");
			break;
		}
        while(1){
            if(j == nthreads){
                j = 0;
            }
            //assign numbers to available workers.
            if(worker[j].status == 0){  
				printf("found active worker in pos %d\n", j);
                break;
            }
            j++;
        }
        printf("pos %d. number %d\n",j, number);
        worker[j].value = number;
        worker[j].status = 1; //changing status to 1 --> worker not available.
	}while(number > 1);
	
    //waiting for all workers to be available.
    flag = 0;
    while(1){
        flag = 0;
        for(i = 0; i < nthreads; i++){
            if(worker[i].status == 1){
                flag = 1;
            }
        }
        if(flag == 0){
            break;
        }
    }

    //changing status to -1 to notify workers to terminate.
    for(i = 0; i < nthreads; i++){
        worker[i].status = -1;
    }

    //waiting for status to be -2 to know that every worker has actually terminated.
    while(1){
        flag =0;
        for(i = 0; i < nthreads; i++){
            if(worker[i].status != -2){
                flag = 1;
            }
            pthread_yield();
        }
        if(flag == 0){
            break;
        }
        pthread_yield();
    }
    
    printf("main is exiting\n");
    return(0);
}
