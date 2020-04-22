#define _GNU_SOURCE
#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>

volatile int size;
volatile int close_val;

struct data_t {
    char c;
    int valid;
};

struct data_t *data = NULL;

void pipe_init(){
	int i;
	
    data = (struct data_t *)malloc(size * sizeof(struct data_t)); // allocate memory
    if(data == NULL){
        perror("malloc error\n");
        exit(0);
    }
    for(i = 0; i < size; i++){
		data[i].valid = 1; //all positions in buffer ready for writing
	}
	
	close_val = 0; //variable used for communication between writer and reader
}

void pipe_close(){
	close_val = 1; // writer will terminate. notify pipe_read that pipe_close has been called
}

void pipe_write(int i, char temp){
	
	while(data[i].valid != 1){ //position is not ready for writing, wait
		pthread_yield();
	}
	data[i].c = temp; //write data
	data[i].valid = 2; //let pipe_read process data
}

void *write_thread(void* ptr){
    int pos, scanf_val;
	void *value = 0;
	char temp;
	
	pos = 0;
	while(1){
		if(pos == size){
			pos = 0;
		}
		scanf_val = scanf("%c", &temp);
		if(scanf_val == EOF){ //end of input
			pipe_close(); //close pipe
			perror("write is exiting\n");
			pthread_exit(value);
		}
		pipe_write(pos, temp); //write temp at pos
		pos++; // next pos
	}
}

int pipe_read(int i, char *temp){
	
	while(data[i].valid != 2){ //position is not ready for reading, wait
		if(close_val == 1){ //if pipe is closed, return 0
			return(0);
		}
		pthread_yield(); // else, wait
	}
    *temp = data[i].c; //read data
	data[i].valid = 1; //let pipe_write process data
    return(1);
}

void *read_thread(void* ptr){
    int i;
    void *retval = 0;
	int *term;
	char data_char;
	int pos, read_val;

	term = (int *)ptr; //term is used to notify main when read_thread is ready to terminate
	
	pos = 0;
	while(1){
		if(pos == size){
			pos = 0;
		}
		read_val = pipe_read(pos, &data_char); //read data and store it to data_char
		if(read_val == 0){ // pipe is closed
			break;
		}
		if(read_val == 1){
			printf("%c", data_char); //print data
		}
		pos++;
	}
	
	close_val = 0; // let pipe_read process data for the last time
	
	//last read from i = pos to size - 1, if necessary
	
	for(i = pos; i < size; i++){
		if(data[i].valid == 2){
			pipe_read(pos, &data_char);
			printf("%c", data[i].c);
		}
	}
	
	//last read from i = 0 to pos - 1, if necessary
	
	for(i = 0; i < pos; i++){
		if(data[i].valid == 2){
			pipe_read(pos, &data_char);
			printf("%c", data[i].c);
		}
	}
	
	free(data); //free allocated memory
	*term = 1; //notify terminate that read_thread has finished
	perror("term  is 1.read is exiting\n");
	pthread_exit(retval);
        
}


int main(int argc, char* argv[]){
    pthread_t thread_id1, thread_id2;
	int term = 0, res;
    
    if(argc != 2){
        perror("Wrong Arguments!\n");
        return(0);
    }
    size = atoi(argv[1]); //size given as argument
    pipe_init();
    // create writer
    res = pthread_create(&thread_id1, NULL, &write_thread, NULL); 
	if(res){
		perror("write error");
	}
	//create reader
    res = pthread_create(&thread_id2, NULL, &read_thread, (void *)&term); 
	if(res){
		perror("read error");
	}
	
    while(1){
        if(term == 1){ //main is waiting for read_thread to terminate
            pthread_yield();
            perror("main is exiting\n");
            break;
        }
        else{
            pthread_yield(); // if not ready yield
        }
    }

    return(0);
}
