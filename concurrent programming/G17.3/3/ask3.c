#define _GNU_SOURCE
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/stat.h>

#define SIZE 100

pthread_cond_t ride, enter, finish, last_pass;
pthread_mutex_t mtx;

struct ride_t{
    int max_pass;
    int curr_pass;
	int waiting;
	int flag;
	int sleeping;
	int out;
    int status;
    int in;
};

void entering(){
    printf("Thread: %lud is entering the train\n", pthread_self());
}

void exiting(){
    printf("Thread: %lud is exiting the train\n", pthread_self());
}

void* train_func(void* ptr){
    struct ride_t *node;
    int ret_val;
    int j = 0;

    node = (struct ride_t*) ptr;

    while(1){
		ret_val = pthread_mutex_lock(&mtx);
		if(ret_val){
			printf("mutex lock error\n");
		}	
		if((node->in < node->max_pass) || (node->status == 0)){
			ret_val = pthread_cond_wait(&ride, &mtx);
			if(ret_val){
				printf("wait error\n");
			}
		}
		
		j++;
		printf("%d ride is going to start! :)\n", j);

		ret_val = pthread_mutex_unlock(&mtx);
		if(ret_val){
			printf("mutex unlock error\n");
		}

		sleep(2);

		ret_val = pthread_mutex_lock(&mtx);
		if(ret_val){
			printf("mutex lock error\n");
		}

		node->flag = 1;

		if(node->sleeping > 0){
			node->sleeping = node->sleeping - 1;
			ret_val = pthread_cond_signal(&finish);
			if(ret_val){
				printf("signal error\n");
			}
		}
		if(node->out < node->max_pass){
			ret_val = pthread_cond_wait(&last_pass, &mtx);
			if(ret_val){
				printf("wait error\n");
			}
		}
		node->in = 0;
		node->status = 0;
		node->curr_pass = 0;
		printf("All passengers out, next ride available :)\n");

		if((node->waiting > 0) && (node->curr_pass < node->max_pass)){
			node->waiting = node->waiting - 1;
			node->curr_pass = node->curr_pass + 1;
			ret_val = pthread_cond_signal(&enter);
			if(ret_val){
				printf("signal error\n");
			}
		}

		node->flag = 0;

		ret_val = pthread_mutex_unlock(&mtx);
		if(ret_val){
			printf("mutex unlock error\n");
		}

	}
}

void* passenger(void* ptr){
    struct ride_t *node;
	int ret_val;

    node = (struct ride_t*) ptr;
	
	ret_val = pthread_mutex_lock(&mtx);
	if(ret_val){
		printf("mutex lock error\n");
	}

	if((node->curr_pass < node->max_pass) && (node->waiting == 0)){
		node->curr_pass = node->curr_pass + 1;
        entering();
        node->in = node->in + 1;
		if(node->in == node->max_pass){;
            node->status = 1;
			ret_val = pthread_cond_signal(&ride);
			if(ret_val){
				printf("signal error\n");
			}
		}
	}
	else{
		node->waiting = node->waiting + 1;
		ret_val = pthread_cond_wait(&enter, &mtx);
		if(ret_val){
			printf("wait error\n");
		}
		while((node->waiting > 0) && (node->curr_pass < node->max_pass)){
            node->waiting = node->waiting - 1;
			node->curr_pass = node->curr_pass + 1;
			ret_val = pthread_cond_signal(&enter);
			if(ret_val){
				printf("signal error\n");
			}
        }
        node->in = node->in + 1;
        entering();
        if(node->in == node->max_pass){
            node->status = 1;
            ret_val = pthread_cond_signal(&ride);
            if(ret_val){
                printf("signal error\n");
            }
        }
	}

	ret_val = pthread_mutex_unlock(&mtx);
	if(ret_val){
		printf("mutex unlock error\n");
	}

// EXITING SECTION!

	ret_val = pthread_mutex_lock(&mtx);
	if(ret_val){
		printf("mutex lock error\n");
	}

	if(node->flag != 1){
		node->sleeping = node->sleeping + 1;
		ret_val = pthread_cond_wait(&finish, &mtx);
		if(ret_val){
			printf("wait error\n");
		}
	}

	if(node->sleeping > 0){
		node->sleeping = node->sleeping - 1;
		ret_val = pthread_cond_signal(&finish);
		if(ret_val){
			printf("signal error\n");
		}
	}

	if(node->out == 0){
		printf("Notified by train that ride has finished :(\n");
	}

	node->out = node->out + 1;
	exiting();

	if(node->out == node->max_pass){
		node->out = 0;
		ret_val = pthread_cond_signal(&last_pass);
		if(ret_val){
			printf("signal error\n");
		}
	}

	ret_val = pthread_mutex_unlock(&mtx);
	if(ret_val){
		printf("mutex unlock error\n");
	}

    return(NULL);
}

int main(int argc, char* argv[]){
    struct ride_t info;
    int res, i;
    pthread_t train_id, passenger_id[SIZE];

    if(argc != 2){
        printf("Wrong Arguments\n");
        return(0);
    }

    info.max_pass = atoi(argv[1]);
    printf("Max Passengers: %d\n", info.max_pass);
	info.flag = 0;
	info.waiting = 0;
	info.curr_pass = 0;
	info.sleeping = 0;
	info.out = 0;
    info.status = 0;
    info.in = 0;
	
	pthread_mutex_init(&mtx, NULL);
	pthread_cond_init(&ride, NULL);
	pthread_cond_init(&last_pass, NULL);
	pthread_cond_init(&finish, NULL);
	pthread_cond_init(&enter, NULL);
	

    //Create train thread.
    res = pthread_create(&train_id, NULL, (void*)&train_func, &info);
    if(res){
        perror("Create Error");
    }

    for(i = 0; i < SIZE; i++){
        res = pthread_create(&passenger_id[i], NULL,(void*) &passenger, &info);
        if(res){
            perror("Create Error");
        }
        printf("created thread with id: %lud\n", passenger_id[i]);
    }

    while(1){
        sleep(3);
        pthread_yield();
    }

    return(0);
}
