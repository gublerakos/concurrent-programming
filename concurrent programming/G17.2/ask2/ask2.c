#include"my_sem.h"

enum boolean{ false, true };

// Struct with 3 variables, value for the input number, status to know if a thread is available or not and thread_id.
struct thread_t{
    pthread_t thread_id;
    int number;
    struct sem_t wait;
    struct sem_t* semid2;
    struct sem_t work;
    struct sem_t* mutex;
    enum boolean term;
    int status;
    int *counter;
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
	int result;

    ptr =(struct thread_t *)ptr1;
    
    while(1){
        mysem_down(&(ptr->wait));
        if(ptr->term == true){
            break;
        }
        result = primetest(ptr->number);
		if(result == 1){
			printf("%d: prime!\n", ptr->number);
		}
		if(result == 0){
			printf("%d: not prime!\n", ptr->number);
		}
        ptr->status = 1;
        mysem_down(ptr->mutex);
        (*(ptr->counter)) = (*(ptr->counter)) + 1;
        
        if((*(ptr->counter)) == 1){
            mysem_up(ptr->semid2);
        }

		mysem_up(ptr->mutex);

        mysem_up(&(ptr->work));
        
        pthread_yield();
    }

    printf("Thread %lud is exiting\n", pthread_self());
    mysem_up(&ptr->work);

	pthread_exit(retval);
}

int main(int argc, char* argv[]){
    int i, nthreads, number, res, counter;
    struct sem_t available, mutex;

	if(argv[1] == NULL){
		return(0);
	}
	// getting the size from args
    nthreads = atoi(argv[1]);
    printf("threads:%d\n", nthreads);
    counter = nthreads;

    struct thread_t worker[nthreads];

    available = mysem_create(1, 0);
    if(available.semid == -1){
        perror("Create Error");
    }

    mutex = mysem_create(1, 1);
    if(mutex.semid == -1){
        perror("Create Error");
    }
	
    // Creating workers.
    for(i = 0; i < nthreads; i++){

        worker[i].wait = mysem_create(1, 0);
        if(worker[i].wait.semid == -1){
            perror("Create Error");
        }
        worker[i].counter = &counter;
        worker[i].semid2 = &available;
        worker[i].mutex = &mutex;

        worker[i].work = mysem_create(1, 1);
        if(worker[i].work.semid == -1){
            perror("Create Error");
        }
        worker[i].nthreads = nthreads;
        worker[i].status = 1;
        worker[i].term = false;
        res = pthread_create(&(worker[i].thread_id), NULL, &worker_func, (void*)&worker[i]);
		printf("created thread id: %lud\n", worker[i].thread_id);
        if(res){
            perror("Error");
        }
    }
    
    //taking numbers with scanf.
	do{
		scanf("%d", &number);
		if(number <= 1){
			printf("exiting\n");
			break;
		}
		//assign numbers to available workers.
        mysem_down(&mutex);
        if(counter == 0){
            mysem_up(&mutex);
            mysem_down(&available);
            //printf("number = %d\n", number);
        }
		else{
            mysem_up(&mutex);
        }
        for(i = 0; i < nthreads; i++){
            if(worker[i].status == 1){
                printf("found active worker in pos %d, number %d\n", i, number);
                worker[i].number = number;
                worker[i].status = 0;
                mysem_down(&(worker[i].work));
                mysem_up(&(worker[i].wait)); //up semaphore with wait to unblock.
                mysem_down(&mutex);
                counter--;
                mysem_up(&mutex);
                break;
            }
        }      
	}while(number > 1);

    for(i = 0; i < nthreads; i++){
        mysem_down(&(worker[i].work));
        worker[i].term = true;
        mysem_up(&(worker[i].wait));
    }

    for(i = 0; i < nthreads; i++){
        mysem_down(&(worker[i].work));
    }

    for(i = 0; i < nthreads; i++){
        mysem_destroy(worker[i].wait);
        mysem_destroy(worker[i].work);
    }
    mysem_destroy(available);
    mysem_destroy(mutex);

    printf("main is exiting\n");
    return(0);
}
