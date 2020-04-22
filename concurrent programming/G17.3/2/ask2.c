#define _GNU_SOURCE
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/stat.h>

enum colour_t {blue, red};

struct car_t{
    int max_cars;
    int curr_bridge;
    enum colour_t colour;
    int red_waiting;
    int blue_waiting;
	int red_passed;
	int blue_passed;
};

pthread_cond_t redq, blueq;
pthread_mutex_t mtx;

void arrive_blue(struct car_t *node){
    int ret_val;

    ret_val = pthread_mutex_lock(&mtx);
    if(ret_val){
        printf("mutex lock error\n");
    }
    
    if(node->colour == blue){
		if((node->curr_bridge == node->max_cars) || ((node->blue_passed >= 2*(node->max_cars)) && (node->red_waiting > 0))){
            node->blue_waiting = node->blue_waiting + 1;
            printf("blue cars on bridge, max capacity (%d), arriving blue and blocked id: %lud\n", node->curr_bridge, pthread_self());
            ret_val = pthread_cond_wait(&blueq, &mtx);
            if(ret_val){
                printf("wait error\n");
            }            
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            printf("blue thread %lud unblocked\n", pthread_self());
        }
        else if(node->curr_bridge < node->max_cars){     //bridge is not full, arriving blue car not blocked.
            node->curr_bridge = node->curr_bridge + 1;
			node->blue_passed = node->blue_passed + 1;
			printf("blue_passed=%d\n",node->blue_passed);
			while((node->blue_waiting > 0) && (node->curr_bridge < node->max_cars)){
				if((node->blue_passed >= 2*(node->max_cars)) && (node->red_waiting > 0)){
					break;
				}
				node->blue_passed = node->blue_passed + 1;
				printf("blue_passed=%d\n",node->blue_passed);
				node->blue_waiting = node->blue_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
		        ret_val = pthread_cond_signal(&blueq);
		        if(ret_val){
			        printf("signal error\n");
		        }
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            printf("blue cars on bridge, arriving blue not blocked, now on bridge: %d\n", node->curr_bridge);
        }
        return;
    }
    else{
        if((node->curr_bridge == 0) && (node->red_waiting == 0)){
            node->colour = blue;
            node->curr_bridge = node->curr_bridge + 1;
			node->blue_passed = 1;
			printf("blue_passed=%d\n",node->blue_passed);
            printf("colour changed to blue (no red cars waiting), now on bridge: %d\n", node->curr_bridge);
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            return;
        }
        else{
            node->blue_waiting = node->blue_waiting + 1;
            printf("red cars on bridge, blue arrived and blocked\n");
            ret_val = pthread_cond_wait(&blueq, &mtx);
            if(ret_val){
                printf("wait error\n");
            }
			while((node->blue_waiting > 0) && (node->curr_bridge < node->max_cars)){
				if((node->blue_passed >= 2*(node->max_cars)) && (node->red_waiting > 0)){
					break;
				}
				node->blue_passed = node->blue_passed + 1;
				printf("blue_passed=%d\n",node->blue_passed);
				node->blue_waiting = node->blue_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
                ret_val = pthread_cond_signal(&blueq);
                if(ret_val){
                    printf("signal error\n");
                }
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            return;
        }
    }
}

void arrive_red(struct car_t *node){
    int ret_val;

    ret_val = pthread_mutex_lock(&mtx);
    if(ret_val){
        printf("mutex lock error\n");
    }

    if(node->colour == red){
		if((node->curr_bridge == node->max_cars) || (node->red_passed >= 2*(node->max_cars))){
            node->red_waiting = node->red_waiting + 1;
            printf("red cars on bridge, max capacity (%d), arriving red and blocked id: %lud\n", node->curr_bridge, pthread_self());
            ret_val = pthread_cond_wait(&redq, &mtx);
            if(ret_val){
                printf("wait error\n");
            }            
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            printf("red thread %lud unblocked\n", pthread_self());
        }
        else if(node->curr_bridge < node->max_cars){     //bridge is not full, arriving red car not blocked.
            node->curr_bridge = node->curr_bridge + 1;
			node->red_passed = node->red_passed + 1;
			printf("red_passed=%d\n",node->red_passed);
			while((node->red_waiting > 0) && (node->curr_bridge < node->max_cars)){
				if((node->red_passed >= 2*(node->max_cars)) && (node->blue_waiting > 0)){
					break;
				}
				node->red_waiting = node->red_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				node->red_passed = node->red_passed + 1;
				printf("red_passed=%d\n",node->red_passed);
                ret_val = pthread_cond_signal(&redq);
		        if(ret_val){
			        printf("signal error\n");
		        }
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
			}
            printf("red cars on bridge, arriving red not blocked, now on bridge: %d\n", node->curr_bridge);
        }
        return;
    }

    else{
        if((node->curr_bridge == 0) && (node->blue_waiting == 0)){
            node->colour = red;
            node->curr_bridge = node->curr_bridge + 1;
			node->red_passed = 1;
			printf("red_passed=%d and blue_waiting = %d\n",node->red_passed, node->blue_waiting);
            printf("colour changed to red (no blue cars waiting), now on bridge: %d\n", node->curr_bridge);
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            return;
        }
        else{
            node->red_waiting = node->red_waiting + 1;
            printf("blue cars on bridge, red arrived and blocked\n");
            ret_val = pthread_cond_wait(&redq, &mtx);
            if(ret_val){
                printf("wait error\n");
            }
			while((node->red_waiting > 0) && (node->curr_bridge < node->max_cars)){
				if((node->red_passed >= 2*(node->max_cars)) && (node->blue_waiting > 0)){
					break;
				}
				node->red_passed = node->red_passed + 1;
				printf("red_passed=%d and blue_waiting = %d\n",node->red_passed, node->blue_waiting);
				node->red_waiting = node->red_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				ret_val = pthread_cond_signal(&redq);
                if(ret_val){
                    printf("signal error\n");
                }
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            return;
        }
    }
}

void leave_blue(struct car_t *node){
    int ret_val;
    ret_val = pthread_mutex_lock(&mtx);
    if(ret_val){
        printf("mutex lock error\n");
    }

    if((node->curr_bridge == node->max_cars) && (node->max_cars > 1)){
        printf("leave_blue:max capacity, curr_bridge = %d\n", node->curr_bridge);
        node->curr_bridge = node->curr_bridge - 1;
        if(node->blue_waiting > 0){
			if(!((node->blue_passed >= (2*(node->max_cars))) && (node->red_waiting > 0))){
				node->blue_waiting = node->blue_waiting - 1;
				node->blue_passed = node->blue_passed + 1;
				printf("blue_passed=%d and red waiting = %d\n",node->blue_passed, node->red_waiting);
				node->curr_bridge = node->curr_bridge + 1;
				ret_val = pthread_cond_signal(&blueq);
				if(ret_val){
					printf("signal error\n");
				}
			}
			ret_val = pthread_mutex_unlock(&mtx);
			if(ret_val){
				printf("mutex unlock error\n");
			}
        }
        else{
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
        }
        return;
    }
    else if(node->curr_bridge == 1){
        printf("blue last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        if(node->red_waiting > 0){
			node->blue_passed = 0;
			node->red_passed = 0;
			printf("red_passed=%d\n",node->red_passed);
			printf("blue_passed=%d\n",node->blue_passed);
			while((node->red_waiting > 0) && (node->curr_bridge < node->max_cars)){
				if(node->red_passed == (2*(node->max_cars))){
					break;
				}
				else{
					node->red_waiting = node->red_waiting - 1;
					node->curr_bridge = node->curr_bridge + 1;
					node->red_passed = node->red_passed + 1;
					printf("red_passed=%d and blue waiting = %d\n",node->red_passed, node->blue_waiting);
					node->colour = red;
					ret_val = pthread_cond_signal(&redq);
					if(ret_val){
						printf("signal error\n");
					}
				}
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
        }
        else if(node->blue_waiting > 0){
			node->blue_passed = 0;
			while((node->blue_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->blue_passed = node->blue_passed + 1;
				printf("blue_passed=%d and red waiting = %d\n",node->blue_passed, node->red_waiting);
				node->blue_waiting = node->blue_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
                ret_val = pthread_cond_signal(&blueq);
                if(ret_val){
                    printf("signal error\n");
                }
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
        }
        else{
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
            return;
        }
    }
    else{
        printf("blue not first not last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        ret_val = pthread_mutex_unlock(&mtx);
        if(ret_val){
            printf("mutex unlock error\n");
        }
        return;
    }
}

void leave_red(struct car_t *node){
    int ret_val;

    ret_val = pthread_mutex_lock(&mtx);
    if(ret_val){
        printf("mutex lock error\n");
    }

    if((node->curr_bridge == node->max_cars) && (node->max_cars > 1)){
        printf("leave_red:max capacity, curr_bridge = %d\n", node->curr_bridge);
        node->curr_bridge = node->curr_bridge - 1;
        if(node->red_waiting > 0){
			if(!((node->red_passed >= (2*(node->max_cars))) && (node->blue_waiting > 0))){
				node->red_passed = node->red_passed + 1;
				printf("red_passed=%d\n",node->red_passed);
				node->red_waiting = node->red_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
				ret_val = pthread_cond_signal(&redq);
				if(ret_val){
					printf("signal error\n");
				}
			}
			ret_val = pthread_mutex_unlock(&mtx);
			if(ret_val){
				printf("mutex unlock error\n");
			}
        }
        else{
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
        }
        return;
    }
    else if(node->curr_bridge == 1){
        printf("red last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        if(node->blue_waiting > 0){
			node->red_passed = 0;
			node->blue_passed = 0;
			printf("blue_passed=%d\n",node->blue_passed);
			printf("red_passed=%d\n",node->red_passed);
			while((node->blue_waiting > 0) && (node->curr_bridge < node->max_cars)){
				if(node->blue_passed == (2*(node->max_cars))){
					break;
				}
				else{
					node->blue_waiting = node->blue_waiting - 1;
					node->curr_bridge = node->curr_bridge + 1;
					node->blue_passed = node->blue_passed + 1;
					printf("blue_passed=%d\n",node->blue_passed);
					node->colour = blue;
					ret_val = pthread_cond_signal(&blueq);
					if(ret_val){
						printf("signal error\n");
					}
				}
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
        }
        else if(node->red_waiting > 0){
			node->red_passed = 0;
			while((node->red_waiting > 0) && (node->curr_bridge < node->max_cars)){
				node->red_passed = node->red_passed + 1;
				printf("red_passed=%d\n",node->red_passed);
				node->red_waiting = node->red_waiting - 1;
				node->curr_bridge = node->curr_bridge + 1;
                ret_val = pthread_cond_signal(&redq);
                if(ret_val){
                    printf("signal error\n");
                }
			}
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
        }
        else{
            ret_val = pthread_mutex_unlock(&mtx);
            if(ret_val){
                printf("mutex unlock error\n");
            }
        }
    }
    else{
        printf("red not first not last car %lud\n", pthread_self());
        node->curr_bridge = node->curr_bridge - 1;
        ret_val = pthread_mutex_unlock(&mtx);
        if(ret_val){
            printf("mutex unlock error\n");
        }
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
    int capacity;
    int flag = 0;
    int res = 0;
    pthread_t blue_thread[200];
    pthread_t red_thread[200];
    struct car_t car_node;
    int i = 0;
    int j = 0;

    pthread_mutex_init(&mtx, NULL);
	pthread_cond_init(&redq, NULL);
	pthread_cond_init(&blueq, NULL);

    printf("Give me the capacity of the bridge: \n");
    scanf("%d", &capacity);
	
    car_node.max_cars = capacity;
    car_node.curr_bridge = 0;
    car_node.blue_waiting = 0;
    car_node.red_waiting = 0;
    car_node.colour = blue;
	car_node.red_passed = 0;
	car_node.blue_passed = 0;

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
