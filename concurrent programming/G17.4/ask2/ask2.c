#define _GNU_SOURCE
#define _POSIX_C_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<ucontext.h>
#include<stdint.h>
#include<malloc.h>
#include<memory.h>
#include<error.h>
#include<limits.h>
#include<signal.h>
#include<sys/time.h>
#include<sys/sem.h>


#define STACK_SIZE 16384

//for thread's status
#define RUNNING 0
#define BLOCKED 1
#define WAITING 2

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int primetest(int number){
    int i;

    for(i = number - 1; i > 1; i--){
        if((number%i) == 0){
            return(0);
        }
    }
    return(1);
}

enum boolean{ false, true };

volatile int exited = 0;

struct info_t{
	int* array;
	enum boolean* flag;
};

typedef struct cor {
    ucontext_t uctx;
    ucontext_t* link;
    ucontext_t* from;
}co_t;

typedef struct semaphore_t {
    int semid;
    int sem_val;
}sem_t;

typedef struct thread_t {
    int thread_id;
    int status;
    int finish;
    int wait_id;
    co_t context;
    sem_t* sem;
    struct thread_t* prev;
    struct thread_t* next;
    struct thread_t* block;
}thr_t;

typedef struct arg_t {
	struct info_t* info;
	int number;
    thr_t* self;
    int nthreads;
}args;

thr_t* sched_thread;
thr_t* main_thread;
thr_t* blocking_head;
thr_t* running_head;

volatile int count = 1;
volatile int id = 0;
volatile int iter = 0;

int mylist_empty(thr_t* head);
thr_t* mylist_add(thr_t* head, thr_t* node);
thr_t* mylist_delete(thr_t* head, thr_t* delete_node);

thr_t* find_running(){
    thr_t* curr;

    if(!mylist_empty(running_head)){
        for(curr = running_head; curr->next != running_head; curr = curr->next){
            if(curr->status == RUNNING){
                    return(curr);
            }
        }
        if(curr->status == RUNNING){
            return(curr);
        }
    }

    return(NULL);
}

int mycoroutines_switchto(co_t *co){

    if(swapcontext(co->from, &(co->uctx)) == -1){
        handle_error("swapcontext");
        return(-1);
    }

    return(0);
}

int mythreads_sem_init(sem_t *s, int val){
    int mask_val;
    sigset_t sig;

    //blocking SIGALRM in order not to be desturbed.
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    mask_val = sigprocmask(SIG_BLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

    s->semid = id;
    id++;

    s->sem_val = val;

    mask_val = sigprocmask(SIG_UNBLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

    return(0);
}

int mythreads_sem_down(sem_t* s){
    int mask_val, retval;
    sigset_t sig;
    thr_t* running_node;

    //blocking SIGALRM in order not to be desturbed.
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    mask_val = sigprocmask(SIG_BLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

    if(s->sem_val > 0){
        s->sem_val = s->sem_val - 1;
    }
    else{
        running_node = find_running();
        if(running_node == NULL){
            printf("empty list\n");
            return(0);
        }
        running_node->status = BLOCKED;

        running_node->sem = s;
        sched_thread->context.from = &(running_node->context.uctx);

        sched_thread->block = running_node;
        retval = mycoroutines_switchto(&(sched_thread->context));
        if(retval == -1){
            perror("switch error");
            return(-1);
        }
    }

    mask_val = sigprocmask(SIG_UNBLOCK, &sig, NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

    return(0);
}

void mylist_print(thr_t* head);

int mylist_empty(thr_t* head){

    if(head == NULL){
        return(1);
    }

    return(0);
}

int mythreads_sem_up(sem_t* s){
    int mask_val;
    sigset_t sig;
    thr_t* curr;

    //blocking SIGALRM in order not to be desturbed.
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    mask_val = sigprocmask(SIG_BLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }
    
    printf("in up func before unblock\n");
    if(s->sem_val > 0){
        s->sem_val = s->sem_val + 1;
        printf("sem %d new val = %d\n", s->semid, s->sem_val);
    }
    else{
        if(!mylist_empty(blocking_head)){
            for(curr = blocking_head; curr->next != blocking_head; curr = curr->next){
                if(curr->sem == s){
                    printf("found one to unblock with id %d\n", curr->thread_id);
                    curr->status = WAITING;
                    curr->sem = NULL;
					blocking_head = mylist_delete(blocking_head, curr);
                    curr->next = curr;
                    curr->prev = curr;
                    printf("new blocking head id = %d\n", blocking_head->thread_id);
                    running_head = mylist_add(running_head, curr);
					mylist_print(blocking_head);
					mask_val = sigprocmask(SIG_UNBLOCK, &sig,NULL);
					if(mask_val == -1){
						perror("sigprocmask error");
						return(-1);
					}
					return(0);
                }
            }
            if(curr->sem == s){
                curr->status = WAITING;
                printf("found one to unblock\n");
                curr->sem = NULL;
                blocking_head = mylist_delete(blocking_head, curr);
                running_head = mylist_add(running_head, curr);
            }
        }
        else{
            s->sem_val = s->sem_val + 1;
        }
    }

    mask_val = sigprocmask(SIG_UNBLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

    return(0);
}

int mythreads_sem_destroy(sem_t* s){
    
    free(s);
    return(0);
}

thr_t* mylist_find_id(thr_t *head, int id){
    thr_t* curr;

    if(!mylist_empty(head)){
        for(curr = head; curr->next != head; curr = curr->next){
            if(curr->thread_id == id){
                    return(curr);
            }
        }
        if(curr->thread_id == id){
            return(curr);
        }
    }

    return(NULL);
}

static void handler(int sig){
    thr_t* running_node;
    int retval;

    running_node = find_running(); //find running node
    if(running_node->next == running_node){ //if only one running return else switch to scheduler
        return;
    }

    printf("alarm ringed, switching to scheduler\n");

    sched_thread->context.from = &(running_node->context.uctx);
    retval = mycoroutines_switchto(&(sched_thread->context));
    if(retval == -1){
        perror("switch error");
        return;
    }
}

void timer_init(){
    struct itimerval t = {{0}};
    struct sigaction act = {{0}};
    int sig_val;

    act.sa_handler = handler;
    act.sa_flags = SA_RESTART;

    //alarm every 0.5 seconds
    t.it_value.tv_sec = 0;
    t.it_value.tv_usec = 500000;
    t.it_interval.tv_sec = 0;
    t.it_interval.tv_usec = 500000;


    sig_val = sigaction(SIGALRM, &act, NULL);
    if(sig_val == -1){
        perror("sigaction error");
        return;
    }

    setitimer(ITIMER_REAL,&t,NULL);
}


/*######################### MY_LIST.H #########################*/

void mylist_init(thr_t* head){
 
    head->next = head;
    head->prev = head;

}

thr_t* mylist_add(thr_t* head, thr_t* node){
    if(head == NULL){
        head = node;
        head->next = head;
        head->prev = head;
        return(head);
    }

    node->prev = head->prev;
    node->next = head;

    head->prev = node;
    node->prev->next = node;

    mylist_print(head);

    return(head);
}

thr_t* mylist_delete(thr_t* head, thr_t* delete_node){

    if(delete_node->next == delete_node){
        head = NULL;
        return(head);
    }
    if(delete_node == head){
        head = head->next;
    }

    delete_node->prev->next = delete_node->next;
    delete_node->next->prev = delete_node->prev;

    return(head);
}

void mylist_print(thr_t *head){
    thr_t *curr;
    int i = 0;
	
	
	if(head == NULL){
		printf("empty list\n");
		return;
	}
	else if(head == blocking_head){
		printf("blocking list:\n");
	}
	else{
		printf("running list:\n");
	}

    if(!mylist_empty(head)){
        for(curr = head; curr->next != head; curr = curr->next){
            printf("id:%d, status = %d\n", curr->thread_id, curr->status);
            i++;
        }
        i++;
        printf("id:%d, status = %d\n", curr->thread_id, curr->status);
        printf("list size = %d\n", i);
    }
}   

/*######################### MY_COROUTINES.H #########################*/

int mycoroutines_init(co_t *main){

    if(getcontext(&(main->uctx)) == -1){
        handle_error("getcontext");
        return(-1);
    }
    return(0);
}

int mycoroutines_create(co_t *co, void (body)(void *), void *arg){

    if(getcontext(&(co->uctx)) == -1){
        handle_error("getcontext");
        return(-1);
    }

    co->uctx.uc_stack.ss_sp = (char*)malloc(STACK_SIZE*sizeof(char));
    if((co->uctx.uc_stack.ss_sp) == NULL){
        return(-1);
    }

    co->uctx.uc_stack.ss_size = STACK_SIZE * sizeof(char);
    co->uctx.uc_link = co->link;
    makecontext(&(co->uctx), (void (*)(void))body, 1, arg);

    return(0);
}



int mycoroutines_destroy(co_t *co){

    free(co->uctx.uc_stack.ss_sp);

    return(0);
}

/*######################### MY_THREADS.H #########################*/

void sched_func(){
    struct sigaction act = {{0}};
    int sig_val, set_val, help = 0;
    int flag = 0;
    thr_t* running_node;
    thr_t* node;
    thr_t* curr; 
    thr_t* temp;
    thr_t* finish_node;

    act.sa_handler = SIG_IGN;
    act.sa_flags = SA_RESTART;

    sig_val = sigaction(SIGALRM, &act, NULL);
    if(sig_val == -1){
        perror("sigaction error");
        return;
    }

    printf("in scheduler\n");
    
    //checking if there is anyone on blocking list that has to wake up!
    if(!mylist_empty(blocking_head)){
        for(curr = blocking_head; curr->next != blocking_head; curr = curr->next){
            if(curr->wait_id != -1){
                temp = mylist_find_id(running_head, curr->wait_id);
                if(temp == NULL){
                    temp = mylist_find_id(blocking_head, curr->wait_id);
                    if(temp == NULL){
                        printf("wake up\n");
                        curr->status = WAITING;
                        curr->wait_id = -1;
                        blocking_head = mylist_delete(blocking_head, curr);
                        running_head = mylist_add(running_head, curr);
                    }
                }
            }
        }
        if(curr->wait_id != -1){
            temp = mylist_find_id(running_head, curr->wait_id);
            if(temp == NULL){
                temp = mylist_find_id(blocking_head, curr->wait_id);
                if(temp == NULL){
                    printf("wake up\n");
                    curr->status = WAITING;
                    curr->wait_id = -1;
                    blocking_head = mylist_delete(blocking_head, curr);
                    running_head = mylist_add(running_head, curr);
                }
            }
        }
    }

    if(!mylist_empty(running_head)){

        for(curr = running_head; curr->next != running_head; curr = curr->next){
            if(curr->finish == 1){
                flag = 1;
                break;
            }
        }
        if(curr->finish == 1){
            flag = 1;
        }
    }
    //finish
    if(flag == 1){
        printf("delete node with id %d\n", curr->thread_id);
        running_node = curr->next;
        if(curr->next == curr){
            for(finish_node = blocking_head; finish_node->next != blocking_head; finish_node = finish_node->next){
                if(finish_node->wait_id == curr->thread_id){
                    blocking_head = mylist_delete(blocking_head, finish_node);
                    running_head = mylist_add(running_head, finish_node);
                    running_node = finish_node;
                    help = 1;
                    break;
                }
            }
            if((finish_node->wait_id == curr->thread_id) && (help == 0)){
                blocking_head = mylist_delete(blocking_head, finish_node);
                running_head = mylist_add(running_head, finish_node);
                running_node = finish_node;
            }
            help = 0;
        }
        running_head = mylist_delete(running_head, curr);
        flag = 0;
        running_node->status = RUNNING;
    }
    //down or join
    else if(sched_thread->block != NULL){
        node = sched_thread->block;
        printf("found thread with id = %d blocked (status = %d), next running node id = %d\n", node->thread_id, node->status, node->next->status);
        running_node = node->next;

        //insert in blocking list
        if(node->status == BLOCKED){
            if(mylist_empty(blocking_head)){
                running_head = mylist_delete(running_head, node);
                blocking_head = node;
                mylist_init(blocking_head);
                printf("adding head to blocking list with id %d\n", blocking_head->thread_id);
            }
            else{
                mylist_print(blocking_head);
                printf("adding node to blocking list(switch) with id = %d\n", node->thread_id);
                running_head = mylist_delete(running_head,node);
                blocking_head = mylist_add(blocking_head, node);
            }
        }
        sched_thread->block = NULL;
        running_node->status = RUNNING;
    }
    else{
        running_node = find_running();
        if(!mylist_empty(running_head)){
            printf("found running with id = %d\n", running_node->thread_id);
			
        }
        running_node->status = WAITING;
        running_node = running_node->next;
        running_node->status = RUNNING;
    }

    printf("node %d now waiting, and node %d now running\n", running_node->prev->thread_id, running_node->thread_id);

    //initialize timer
    timer_init();
    //switch coroutine
    running_node->context.from = &(sched_thread->context.uctx);
    set_val = setcontext(&(running_node->context.uctx));
    if(set_val == -1){
        perror("set error");
        return;
    }
}

int mythreads_init(){

    sched_thread = (thr_t*)malloc(sizeof(thr_t));
    if(sched_thread == NULL){
        perror("malloc error");
        return(-1);
    }

    sched_thread->thread_id = 0;
    sched_thread->wait_id = -1;
    sched_thread->sem = NULL;

    sched_thread->context.link = NULL;
    mycoroutines_create(&(sched_thread->context), &sched_func, NULL);

    main_thread = (thr_t*)malloc(sizeof(thr_t));
    if(main_thread == NULL){
        perror("malloc error");
        return(-1);
    }

    running_head = main_thread;
    mylist_init(running_head);
    mycoroutines_init(&(main_thread->context));
    main_thread->status = RUNNING;
    main_thread->thread_id = count;
    count++;
    main_thread->wait_id = -1;
    main_thread->sem = NULL;

    blocking_head = NULL;

    timer_init();

    return(0);
}

int mythreads_create(thr_t* node, void (body)(void *), void *arg){
	int ret_val, mask_val;
    sigset_t sig;
	
    //blocking SIGALRM in order not to be desturbed while creating a thread.
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    mask_val = sigprocmask(SIG_BLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

    node->status = WAITING;
    node->thread_id = count;
    count++;
    node->wait_id = -1;
    node->sem = NULL;
    node->context.link = &(sched_thread->context.uctx);
    running_head = mylist_add(running_head, node);
    ret_val = mycoroutines_create(&(node->context), body, arg);
	if(ret_val == -1){
		printf("create error\n");
		return(-1);
	}

    printf("created thread with id = %d, status = %d\n", node->thread_id, node->status);

    mask_val = sigprocmask(SIG_UNBLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

	return(0);
}

int mythreads_yield(){
    thr_t* running_node;
    int mask_val, ret_val;
    sigset_t sig;
	
    //blocking SIGALRM in order not to be desturbed while yielding.
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    mask_val = sigprocmask(SIG_BLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }
    printf("yield\n");
    running_node =find_running();

    if(running_node->next == running_node){
        mask_val = sigprocmask(SIG_UNBLOCK, &sig,NULL);
        if(mask_val == -1){
            perror("sigprocmask error");
            return(-1);
        }
        return(0);
    }

    sched_thread->context.from = &(running_node->context.uctx);
    ret_val = mycoroutines_switchto(&(sched_thread->context));
    if(ret_val == -1){
        perror("switch error");
        return(-1);
    }
    return(0);
}

int mythreads_join(thr_t* node){
    int mask_val;
    sigset_t sig;
    thr_t *wait_node;
    thr_t* running_node;
    int ret_val;
	
    //blocking SIGALRM in order not to be desturbed.
    sigemptyset(&sig);
    sigaddset(&sig, SIGALRM);
    mask_val = sigprocmask(SIG_BLOCK, &sig,NULL);
    if(mask_val == -1){
        perror("sigprocmask error");
        return(-1);
    }

    running_node = find_running();
    if(running_node == NULL){
        return(-1);
    }

    wait_node = mylist_find_id(running_head, node->thread_id);
    if(wait_node == NULL){
        wait_node = mylist_find_id(blocking_head, node->thread_id);
        if(wait_node == NULL){
            printf("node with id %d has finished\n", node->thread_id);
            mask_val = sigprocmask(SIG_UNBLOCK, &sig,NULL);
            if(mask_val == -1){
                perror("sigprocmask error");
                return(-1);
            }
            return(0);
        }
    }
    printf("node with id %d has not finished, id=%d going to block\n", node->thread_id, running_node->thread_id);
    running_node->wait_id = wait_node->thread_id;
    running_node->status = BLOCKED;

    sched_thread->context.from = &(running_node->context.uctx);
    sched_thread->block = running_node;

    ret_val = mycoroutines_switchto(&(sched_thread->context));
    if(ret_val == -1){
        perror("switch error");
        return(-1);
    }
    
    return(0);
}

int mythreads_destroy(thr_t *node){

    if(node == NULL){
        return(-1);
    }

    free(node);

    return(0);
}

sem_t* available;
sem_t* job;

void worker_func(void *ptr1){
    args* ptr;
	int result, i;

    ptr =(args*)ptr1;
	printf("Worker %d created\n", ptr->self->thread_id);
    while(1){
        mythreads_sem_down(job);

        if(exited == 1){
            printf("notified by main to terminate\n");
            for(i = 0; i < (ptr->nthreads); i++){
                if(ptr->info->flag[i] == 1){
                    ptr->number = ptr->info->array[i];
                    ptr->info->flag[i] = 0;
                    printf("got number %d, id=%d", ptr->number, ptr->self->thread_id);
                    result = primetest(ptr->number);
                    if(result == 1){
                        printf("-----------------------%d: prime!\n",ptr->number);
                    }
                    if(result == 0){
                        printf("-----------------------%d: not prime!\n", ptr->number);
                    }
                    iter++;
                    break;
                }
            }  
            break;
        }

        for(i = 0; i < (ptr->nthreads); i++){
			if(ptr->info->flag[i] == 1){
				ptr->number = ptr->info->array[i];
                ptr->info->array[i] = 0;
				ptr->info->flag[i] = 0;
				break;
			}
		}
        iter++;
        printf("got number %d, id=%d", ptr->number, ptr->self->thread_id);
		result = primetest(ptr->number);
		if(result == 1){
			printf("-----------------------%d: prime!\n", ptr->number);
		}
		if(result == 0){
			printf("-----------------------%d: not prime!\n", ptr->number);
		}
        mythreads_sem_up(available);
	}

    printf("finished id = %d\n", ptr->self->thread_id);
    ptr->self->finish = 1;
    return;
}

/*######################### MAIN #########################*/



int main(int argc, char *argv[]){
	int nthreads = 0;
	thr_t *worker_array;
	struct info_t* info;
	int ret_val = 0;
	int i, number, init_val = 0;
	
	if(argc != 2){
		printf("Wrong arguments\n");
		return(0);
	}
	
	nthreads = atoi(argv[1]);
	printf("%d threads\n", nthreads);
	
	worker_array = (thr_t *)malloc(nthreads * sizeof(thr_t));
	if(worker_array == NULL){
		printf("malloc error\n");
		return(0);
	}
	
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

	printf("main started\n");
	args worker_args[nthreads];

    job = (sem_t*)malloc(sizeof(sem_t));
    if(job == NULL){
        perror("malloc error");
        return(0);
    }

    init_val = mythreads_sem_init(job, 0);
    if(init_val == -1){
        perror("mythreads_sem_init error");
        return(0);
    }

    available = (sem_t*)malloc(sizeof(sem_t));
    if(available == NULL){
        perror("malloc error");
        return(0);
    }

    init_val = mythreads_sem_init(available, nthreads);
    if(init_val == -1){
        perror("mythreads_sem_init error");
        return(0);
    }

    init_val = mythreads_init();
    if(init_val == -1){
        perror("mythreads_init error");
        return(0);
    }
	for(i = 0; i < nthreads; i++){
		info->array[i] = 0;
		info->flag[i] = 0;
		worker_args[i].info = info;
		worker_args[i].number = 0;
        worker_args[i].self = &worker_array[i];
        worker_args[i].self->finish = 0;
        worker_args[i].nthreads = nthreads;
		
		ret_val = mythreads_create(&worker_array[i], &worker_func, (void*)&worker_args[i]);
		if(ret_val == -1){
			return(0);
		}

        worker_args[i].self = &worker_array[i];
	}
    //taking numbers with scanf.
	while(1){
        printf("GIVE NUMBER\n");
		scanf("%d", &number);
		if(number <= 1){
            exited = 1;
            break;
        }
        else{
    
            //assign job, if no one available main blocks.
            mythreads_sem_down(available);
            for(i = 0; i < nthreads; i++){
                if(worker_args[i].info->flag[i] == 0){
                    worker_args[i].info->array[i] = number;
                    worker_args[i].info->flag[i] = 1;
                    break;
                }
		    }
            mythreads_sem_up(job);
        }
        mythreads_yield();
    }

    for(i = 0; i < nthreads; i++){
        printf("unblocking thread %d\n", i+2);
        mythreads_sem_up(job);
    }

    for(i = 0; i < nthreads; i++){
        printf("waiting for thread %d\n", i+2);
        mythreads_join(&(worker_array[i]));
    }

    free(info->array);
	free(info->flag);
	free(info);

    mythreads_sem_destroy(job);
    mythreads_sem_destroy(available);

    ret_val = mythreads_destroy(worker_array);
    if(ret_val == -1){
        perror("destroy error");
    }


    printf("iter %d\n", iter);
    free(sched_thread);
    free(main_thread);

    return(0);
}
