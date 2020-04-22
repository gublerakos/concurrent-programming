#define _GNU_SOURCE
#include <pthread.h>
#include<stdio.h>
#include<stdlib.h>

#define SIZE 10

int array[SIZE];

struct worker_t{
    int right, left, ready;
};

void print(int end){
    int i;
    for(i = 0; i < end; i++){
        printf("%d\n", array[i]); //print sorted array
    }
}

void swap(int i, int j){
    int temp;

    temp = array[i];
    array[i] = array[j];
    array[j] = temp;
}

void *quicksort(void *ptr){
    int i, j;
    int pivot;
    int res1, res2;
    struct worker_t *curr;
    struct worker_t *left_child;
    struct worker_t *right_child;
    pthread_t left_id, right_id;

    curr = (struct worker_t*)ptr;
    pivot = array[(curr->right)]; //init pivot
    i = (curr->left) - 1;
    j = curr->right;
    
    while(1){
        while(array[++i] < pivot){ //find element > array[right] or i == right
        }
        while(pivot < array[--j]){ //find element > array[right] or j == left
            if(j == curr->left){
                break;
            }
        }
        if(i>=j){ //i is bigger or equal to j, break
            break;
        }
        swap(i,j);
    }
    
    swap(i, (curr->right)); //pivot is placed to pos i
    
    if((curr->left) < (i - 1)){ //condition to make left child
        left_child = (struct worker_t *)malloc(sizeof(struct worker_t));
        if(left_child == NULL){
            perror("malloc error\n");
        }
        left_child->left = curr->left;
        left_child->right = i - 1;
        left_child->ready = 0;
        res1 = pthread_create(&left_id,NULL, &quicksort, (void*)left_child);
        if(res1){
            perror("Left error");
        }
    }
    if((i + 1) < (curr->right)){ //condition to make right child
        right_child = (struct worker_t *)malloc(sizeof(struct worker_t));
        if(right_child == NULL){
            perror("malloc error\n");
        }
        right_child->left = i + 1;
        right_child->right = curr->right;
        right_child->ready = 0;
        res2 = pthread_create(&right_id,NULL, &quicksort, (void*)right_child);
        if(res2){
            perror("Right error");
        }
    }

    if(((curr->left) < (i - 1)) && ((i + 1) < (curr->right))){ //if there are 2 childs
        while(1){
            if(right_child->ready == 1 && left_child->ready == 1){ //if both childs are ready, terminate
				free(right_child);
				free(left_child);
                curr->ready = 1; //ready to terminate
                break;
            }
            else{
                pthread_yield();
            }
        }
    }
    else if((curr->left) < (i - 1)){ //if there is only left child
        while(1){
            if(left_child->ready == 1){ //if left child ready, terminate
				free(left_child);
                curr->ready = 1; //ready to terminate
                break;
            }
            else{
                pthread_yield();
            }
        }
    }
    else if((i + 1) < (curr->right)){ //if there is only right child
        while(1){
            if(right_child->ready == 1){ //if right child ready, terminate
				free(right_child);
                curr->ready = 1; // ready to terminate
                break;
            }
            else{
                pthread_yield();
            }
        }
    }
    else{
        curr->ready = 1; //ready to terminate
    }
    return(0);
}

int main(int argc, char*argv[]){
    struct worker_t worker;
    pthread_t worker_id;
	int scanf_val, number, i, end, res;
	
	i = 0;
	
	while(1){
		scanf_val = scanf("%d", &number);
        if((scanf_val == EOF) && (i == 0)){ //no input given
            printf("no input\n");
            return(0);
        }
		if((scanf_val == EOF) || (i == SIZE)){ //end of input or max numbers
			worker.right = i - 1; //adjust right
			end = i;
			break;
		}
		array[i] = number;
		i++;
	}
	
	//init first worker
    worker.left = 0;
    worker.ready = 0;

    res = pthread_create(&worker_id, NULL, &quicksort, (void*)&worker);
    if(res){
        perror("Error");
        return(0);
    }

    while(1){
        if(worker.ready == 1){ //if first worker has finished, break
            break;
        }
        else{
            pthread_yield(); //else, yield
        }
    }

    print(end); //print sorted array
  
    return(0);
}
