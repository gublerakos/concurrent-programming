#include"my_sem.h"

struct sem_t mysem_create(int number, int init_val){
    int ctl_val;
	struct sem_t info;

    info.semid = semget(IPC_PRIVATE, number, S_IRWXU);
    if(info.semid == -1){
        perror("Semget Error");
        return(info);
    }

    printf("Semid = %d\n", info.semid);

    ctl_val = semctl(info.semid, 0, SETVAL, init_val);
    if(ctl_val == -1){
        perror("Semctl Error");
    }
	
	info.mtxid = semget(IPC_PRIVATE, number, S_IRWXU);
    if(info.mtxid == -1){
        perror("Semget Error");
        return(info);
    }
    
    ctl_val = semctl(info.mtxid, 0, SETVAL, 1);
    if(ctl_val == -1){
        perror("Semctl Error");
    }

    return(info);
}

int mysem_up(struct sem_t *info){
    struct sembuf op, op_mtx;
    int ret_val;
	
	//down(mtx)
	op_mtx.sem_num = 0;
	op_mtx.sem_op = -1;
	op_mtx.sem_flg = 0;
	ret_val = semop(info->mtxid, &op_mtx, 1); 
	if(ret_val == -1){
		perror("semop error");
		return(-2);
	}	
    //if sem is up, returns 1.
    if(semctl(info->semid, 0, GETVAL)){
        perror("sem is already up");
        return(-1);
    }
    else{
		//up semid
        op.sem_num = 0;
        op.sem_op = 1;
        op.sem_flg = 0;
        ret_val = semop(info->semid, &op, 1);
        if(ret_val == -1){
            perror("semop error");
            return(-2);
        }
    }
    //up (mtx)
    op_mtx.sem_num = 0;
    op_mtx.sem_op = 1;
    op_mtx.sem_flg = 0;
    ret_val = semop(info->mtxid, &op_mtx, 1); 
    if(ret_val == -1){
        perror("semop error");
        return(-2);
    }
    return(0);
}

int mysem_down(struct sem_t *info){
    struct sembuf op;
    int ret_val;
		
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    ret_val = semop(info->semid, &op, 1);
    if(ret_val == -1){
        perror("semop error");
        return(-2);
    }

    return(0);
} 

void mysem_destroy(struct sem_t info){
    int ctl_val;

    ctl_val = semctl(info.semid, 0, IPC_RMID);
    if(ctl_val == -1){
        perror("Rm Error");
    }
    
    ctl_val = semctl(info.mtxid, 0, IPC_RMID);
    if(ctl_val == -1){
        perror("Rm Error");
    }
}
