#include"my_sem.h"

enum colour_t {blue, red};

struct car_t{
    int max_cars;
    int curr_bridge;
    enum colour_t colour;
    int red_waiting;
    int blue_waiting;
    struct sem_t mutex, redq, blueq;
};

void arrive_blue(struct car_t *node){
    mysem_down(&(node->mutex));
    if(node->colour == blue){
        if(node->curr_bridge < node->max_cars){     //bridge is not full, arriving blue car not blocked.
            node->curr_bridge = node->curr_bridge + 1;
			while((node->blue_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->blue_waiting = node->blue_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				mysem_up(&(node->blueq));
			}
            printf("blue cars on bridge, arriving blue not blocked, now on bridge: %d\n", node->curr_bridge);
            mysem_up(&(node->mutex));
        }
        else if(node->curr_bridge == node->max_cars){
            node->blue_waiting = node->blue_waiting + 1;
            printf("blue cars on bridge, max capacity (%d), arriving blue and blocked id: %lud\n", node->curr_bridge, pthread_self());
            mysem_up(&(node->mutex));
            mysem_down(&(node->blueq));
            printf("thread %lud unblocked\n", pthread_self());
        }
        return;
    }
    else{
        if((node->curr_bridge == 0) && (node->red_waiting > 0)){
            node->red_waiting = node->red_waiting - 1;
            node->curr_bridge = node->curr_bridge + 1;
            printf("bridge found empty, red cars are waiting, now on bridge: %d\n", node->curr_bridge);
            mysem_up(&(node->mutex));
            mysem_up(&(node->redq));
            return;
        }
        else if((node->curr_bridge == 0) && (node->red_waiting == 0)){
            node->colour = blue;
            node->curr_bridge = node->curr_bridge + 1;
            printf("colour changed to blue (no red cars waiting), now on bridge: %d\n", node->curr_bridge);
            mysem_up(&(node->mutex));
            return;
        }
        else{
            node->blue_waiting = node->blue_waiting + 1;
            mysem_up(&(node->mutex));
            printf("red cars on bridge, blue arrived and blocked\n");
            mysem_down(&(node->blueq));
			mysem_down(&(node->mutex));
			while((node->blue_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->blue_waiting = node->blue_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				mysem_up(&(node->blueq));
			}
			mysem_up(&(node->mutex));
            return;
        }
    }
}

void arrive_red(struct car_t *node){
    mysem_down(&(node->mutex));
    if(node->colour == red){
        if(node->curr_bridge < node->max_cars){     //bridge is not full, arriving red car not blocked.
            node->curr_bridge = node->curr_bridge + 1;
			while((node->red_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->red_waiting = node->red_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				mysem_up(&(node->redq));
			}
            printf("red cars on bridge, arriving red not blocked, now on bridge: %d\n", node->curr_bridge);
            mysem_up(&(node->mutex));
        }
        else if(node->curr_bridge == node->max_cars){
            node->red_waiting = node->red_waiting + 1;
            printf("red cars on bridge, max capacity (%d), arriving red and blocked id: %lud\n", node->curr_bridge, pthread_self());
            mysem_up(&(node->mutex));
            mysem_down(&(node->redq));
            printf("thread %lud unblocked\n", pthread_self());
        }
        return;
    }
    else{
        if((node->curr_bridge == 0) && (node->blue_waiting > 0)){
            node->blue_waiting = node->blue_waiting - 1;
            node->curr_bridge = node->curr_bridge + 1;
            printf("bridge found empty, blue cars are waiting, now on bridge: %d\n", node->curr_bridge);
            mysem_up(&(node->mutex));
            mysem_up(&(node->blueq));
            return;
        }
        else if((node->curr_bridge == 0) && (node->blue_waiting == 0)){
            node->colour = red;
            node->curr_bridge = node->curr_bridge + 1;
            printf("colour changed to red (no blue cars waiting), now on bridge: %d\n", node->curr_bridge);
            mysem_up(&(node->mutex));
            return;
        }
        else{
            node->red_waiting = node->red_waiting + 1;
            mysem_up(&(node->mutex));
            printf("blue cars on bridge, red arrived and blocked\n");
            mysem_down(&(node->redq));
			mysem_down(&(node->mutex));
			while((node->red_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->red_waiting = node->red_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				mysem_up(&(node->redq));
			}
			mysem_up(&(node->mutex));
            return;
        }
    }
}

void leave_blue(struct car_t *node){
    mysem_down(&(node->mutex));
    if(node->curr_bridge == node->max_cars){
        printf("leave_blue:max capacity\n");
        node->curr_bridge = node->curr_bridge - 1;
        if(node->blue_waiting > 0){
            node->blue_waiting = node->blue_waiting - 1;
            node->curr_bridge = node->curr_bridge + 1;
            mysem_up(&(node->mutex));
            mysem_up(&(node->blueq));
        }
        else{
            mysem_up(&(node->mutex));
        }
        return;
    }
    else if(node->curr_bridge == 1){
        printf("blue last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        if(node->red_waiting > 0){
            node->red_waiting = node->red_waiting - 1;
            node->curr_bridge = node->curr_bridge + 1;
            node->colour = red;
            mysem_up(&(node->mutex));
            mysem_up(&(node->redq));
        }
        else if(node->blue_waiting > 0){
			while((node->blue_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->blue_waiting = node->blue_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				mysem_up(&(node->blueq));
			}
            mysem_up(&(node->mutex));
        }
        else{
            mysem_up(&(node->mutex));
            return;
        }
    }
    else{
        printf("blue not first not last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        mysem_up(&(node->mutex));
        return;
    }
}

void leave_red(struct car_t *node){
    printf("%lud exiting bridge before mtx\n", pthread_self());
    mysem_down(&(node->mutex));
    printf("%lud exiting bridge after mtx\n", pthread_self());
    if(node->curr_bridge == node->max_cars){
        printf("leave_red:max capacity\n");
        node->curr_bridge = node->curr_bridge - 1;
        if(node->red_waiting > 0){
            node->red_waiting = node->red_waiting - 1;
            node->curr_bridge = node->curr_bridge + 1;
            mysem_up(&(node->mutex));
            mysem_up(&(node->redq));
        }
        else{
            mysem_up(&(node->mutex));
        }
        return;
    }
    else if(node->curr_bridge == 1){
        printf("red last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        if(node->blue_waiting > 0){
            node->blue_waiting = node->blue_waiting - 1;
            node->curr_bridge = node->curr_bridge + 1;
            node->colour = blue;
            mysem_up(&(node->mutex));
            mysem_up(&(node->blueq));
        }
        else if(node->red_waiting > 0){
			while((node->red_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->red_waiting = node->red_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				mysem_up(&(node->redq));
			}
            mysem_up(&(node->mutex));
        }
        else{
            mysem_up(&(node->mutex));
        }
    }
    else{
        printf("red not first not last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        mysem_up(&(node->mutex));
    }
}

void* blue_func(void *args){
    struct car_t *ptr;

    ptr = (struct car_t *)args;
    arrive_blue(ptr);
    printf("blue %lud on bridge\n", pthread_self());
	sleep(1);
    leave_blue(ptr);


    return(NULL);
}

void* red_func(void *args){
    struct car_t *ptr;

    ptr = (struct car_t *)args;

    arrive_red(ptr);
	sleep(1);
    printf("red %lud on bridge\n", pthread_self());
    leave_red(ptr);

    return(NULL);
}

int main(int argc, char* argv[]){
    int capacity, flag = 0;
    int res = 0;
    pthread_t blue_thread[200];
    pthread_t red_thread[200];
    struct car_t car_node;
    int i = 0;
    int j = 0;

    printf("Give me the capacity of the bridge: \n");
    scanf("%d", &capacity);
	
    car_node.max_cars = capacity;
    car_node.curr_bridge = 0;
    car_node.blueq = mysem_create(1, 0);
    car_node.blue_waiting = 0;
    car_node.mutex = mysem_create(1, 1);
    car_node.red_waiting = 0;
    car_node.redq = mysem_create(1, 0);
    car_node.colour = blue;
	
    while(1){
        if((rand()%2) == 0){
            if(i == 10){
                break;
            }
            if(flag == 0){    
                car_node.colour = blue;
                flag = 1;
            }
            res = pthread_create(&(blue_thread[i]), NULL, (void *)&blue_func, &car_node);
            if(res){
                perror("blue error");
            }
            i++;
        }
        else{
            if(j == 10){
                break;
            }
            if(flag == 0){    
                car_node.colour = red;
                flag = 1;
            }
            res = pthread_create(&(red_thread[j]), NULL, (void *)&red_func,&car_node);
            if(res){
                perror("red error");
            }
            j++;
        }
    }

    while(1){
        pthread_yield();
        sleep(3);
        printf("%d cars on bridge right now, %d blue waiting, %d red waiting\n", car_node.curr_bridge, car_node.blue_waiting, car_node.red_waiting);
    }
    return(0);
}
