#include"my_sem.h"

#define SIZE 23

struct ride_t{
    struct sem_t ride;
    struct sem_t wait;
    struct sem_t mtx;
    struct sem_t out;
    struct sem_t in;
    struct sem_t finish;
    struct sem_t down;
    int max_pass;
    int curr_pass;
};

void entering(){
    printf("Thread: %lud is entering the train\n", pthread_self());
}

void exiting(){
    printf("Thread: %lud is exiting the train\n", pthread_self());
}

void* train(void* ptr){
    struct ride_t *node;
    int i;
    int j = 0;

    node = (struct ride_t*) ptr;

    while(1){
        mysem_down(&(node->ride));
        j++;
        printf("%d ride is going to start! :)\n", j);

        for(i = 0; i < (node->max_pass); i++){
            mysem_up(&(node->wait));
            mysem_down(&(node->in));
        }
        //ride starts here
        printf("############################################################\n");
        sleep(1);
        //ride ends here

        for(i = 0; i < (node->max_pass); i++){
            mysem_up(&(node->out));
            mysem_down(&(node->down));
        }
        printf("############################################################\n");        
        mysem_up(&(node->finish));
    }
}

void* passenger(void* ptr){
    struct ride_t *node;

    node = (struct ride_t*) ptr;

    mysem_down(&(node->mtx));
    node->curr_pass = node->curr_pass + 1;
    if(node->curr_pass == node->max_pass){
        node->curr_pass = 0;
        mysem_up(&(node->mtx));
        mysem_down(&(node->finish));
        mysem_up(&(node->ride));
    }
    else{
        mysem_up(&(node->mtx));
    }
    mysem_down(&(node->wait));
    entering();
    mysem_up(&(node->in));
    mysem_down(&(node->out));
    exiting();
    mysem_up(&(node->down));

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

    info.ride = mysem_create(1, 0);
    info.wait = mysem_create(1, 0);
    info.mtx = mysem_create(1, 1);
    info.out = mysem_create(1, 0);  
    info.in = mysem_create(1, 0);
    info.finish = mysem_create(1, 1);
    info.down = mysem_create(1, 0);

    //Create train thread.
    res = pthread_create(&train_id, NULL, (void*)&train, &info);
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
        printf("here\n");
        pthread_yield();
    }

    return(0);
}
