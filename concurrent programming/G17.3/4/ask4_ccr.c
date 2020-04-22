#define _GNU_SOURCE
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/stat.h>

#define SIZE 15

#define CCR_DECLARE(label)\
	static pthread_cond_t R_q1##label;\
	static pthread_cond_t R_q2##label;\
	static pthread_mutex_t R_mtx##label;\
	static pthread_cond_t enter##label;\
	static int retval##label;\
	static int R_n1##label;\
	static int sleeping##label;\
	static int status##label;\
	static int R_n2##label;

#define CCR_INIT(label)\
	pthread_mutex_init(&R_mtx##label, NULL);\
	pthread_cond_init(&R_q1##label, NULL);\
	pthread_cond_init(&R_q2##label, NULL);\
	pthread_cond_init(&enter##label, NULL);\
	R_n1##label = 0;\
	sleeping##label = 0;\
	status##label = 1;\
	R_n2##label = 0;

#define CCR_EXEC(label,cond, body)\
	printf("%lud before mtx\n", pthread_self());\
	retval##label = pthread_mutex_lock(&R_mtx##label);\
	if(retval##label){\
		printf("lock error\n");\
	}\
	sleeping##label++;\
	if(status##label == 0){\
		printf("%lud blocking to enter queue\n", pthread_self());\
		retval##label = pthread_cond_wait(&enter##label, &R_mtx##label);\
		if(retval##label){\
			printf("signal error\n");\
		}\
	}\
	else if(sleeping##label > 1){\
		retval##label = pthread_cond_wait(&enter##label, &R_mtx##label);\
		if(retval##label){\
			printf("signal error\n");\
		}\
	}\
	else{\
		sleeping##label--;\
	}\
	printf("%lud after mtx\n", pthread_self());\
	status##label = 0;\
	while(!cond){\
		R_n1##label++;\
		if(R_n2##label > 0){\
			R_n2##label--;\
			printf("%lud signal q2\n", pthread_self());\
			retval##label = pthread_cond_signal(&R_q2##label);\
			if(retval##label){\
				printf("signal error\n");\
			}\
		}\
		else{\
			status##label = 1;\
			if(sleeping##label > 0){\
				sleeping##label--;\
				printf("%lud signal enter, n1 = %d, n2 = %d\n", pthread_self(), R_n1##label, R_n2##label);\
				retval##label = pthread_cond_signal(&enter##label);\
				if(retval##label){\
					printf("signal error\n");\
				}\
			}\
		}\
		printf("%lud blocking to q1\n", pthread_self());\
		retval##label = pthread_cond_wait(&R_q1##label, &R_mtx##label);\
		if(retval##label){\
			printf("wait error\n");\
		}\
		R_n2##label++;\
		if(R_n1##label > 0){\
			R_n1##label--;\
			printf("%lud signal q1\n", pthread_self());\
			retval##label = pthread_cond_signal(&R_q1##label);\
			if(retval##label){\
				printf("signal error\n");\
			}\
		}\
		else{\
			R_n2##label--;\
			if(R_n2##label > 0){\
				printf("%lud signal q2, n2 = %d\n", pthread_self(), R_n2##label);\
				retval##label = pthread_cond_signal(&R_q2##label);\
				if(retval##label){\
					printf("signal error\n");\
				}\
			}\
		}\
		if(R_n2##label >= 1){\
			printf("%lud blocking to q2\n", pthread_self());\
			retval##label = pthread_cond_wait(&R_q2##label, &R_mtx##label);\
			if(retval##label){\
				printf("wait error\n");\
			}\
		}\
		else{\
            printf("only one in q2(not blocked) and going to q1\n");\
        }\
	}\
	printf("%lud in CS\n", pthread_self());\
	body;\
	if(R_n1##label > 0){\
		R_n1##label--;\
		printf("%lud signal q1\n", pthread_self());\
		retval##label = pthread_cond_signal(&R_q1##label);\
		if(retval##label){\
			printf("signal error\n");\
		}\
	}\
	else if(R_n2##label > 0) {\
		R_n2##label--;\
		printf("%lud signal q2\n", pthread_self());\
		retval##label = pthread_cond_signal(&R_q2##label);\
		if(retval##label){\
			printf("signal error\n");\
		}\
	}\
	else{\
		status##label = 1;\
		if(sleeping##label > 0){\
			sleeping##label--;\
			printf("%lud signal enter\n", pthread_self());\
			retval##label = pthread_cond_signal(&enter##label);\
			if(retval##label){\
				printf("signal error\n");\
			}\
		}\
	}\
	retval##label = pthread_mutex_unlock(&R_mtx##label);\
	if(retval##label){\
		printf("unlock error\n");\
	}\

struct ride_t{
    int max_pass;
    int curr_pass;
	int flag;
	int out;
};

CCR_DECLARE(1);

void entering(){
    printf("Thread: %lud is entering the train\n", pthread_self());
}

void exiting(){
    printf("Thread: %lud is exiting the train\n", pthread_self());
}

void* train_func(void* ptr){
    struct ride_t *node;
    int j = 0;

    node = (struct ride_t*) ptr;

	printf("Train came, GET READY!!!\n");

    while(1){
		CCR_EXEC(1, (node->curr_pass == node->max_pass), printf("Max passengers entered train\n"));

		j++;
		printf("%d ride is going to start! :)\n", j);
		sleep(2); //ride for 2 seconds.

		CCR_EXEC(1, 1, node->flag = 1);

		CCR_EXEC(1, (node->out == node->max_pass), node->curr_pass = 0; node->out = 0;printf("(exiting)curr_pass = %d\n", node->curr_pass); node->flag = 0);
	}
}

void* passenger(void* ptr){
    struct ride_t *node;

    node = (struct ride_t*) ptr;
	
	CCR_EXEC(1, (node->curr_pass < node->max_pass), node->curr_pass++; entering());

	CCR_EXEC(1, (node->flag == 1), node->out++; exiting());

	return(NULL);
}

int main(int argc, char* argv[]){
    struct ride_t info;
    int res, i;
    pthread_t train_id, passenger_id[SIZE];

	CCR_INIT(1);

    if(argc != 2){
        printf("Wrong Arguments\n");
        return(0);
    }

    info.max_pass = atoi(argv[1]);
    printf("Max Passengers: %d\n", info.max_pass);
	info.flag = 0;
	info.curr_pass = 0;
	info.out = 0;
	
    //Create train thread.
    res = pthread_create(&train_id, NULL, (void*)&train_func, &info);
    if(res){
        perror("Create Error");
    }
	printf("train id is %lud\n", train_id);

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
