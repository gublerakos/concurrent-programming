#define _GNU_SOURCE
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/stat.h>

#ifndef __MY_SEM_H__
#define __MY_SEM_H__

struct sem_t{
	int semid;
	int mtxid;
};

struct sem_t mysem_create(int number, int init_val);

int mysem_up(struct sem_t *info);

int mysem_down(struct sem_t *info);

void mysem_destroy(struct sem_t info);

#endif
