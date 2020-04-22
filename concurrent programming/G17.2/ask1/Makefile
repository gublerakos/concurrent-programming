all: 	ask2 ask3 ask4

###################### ask2 ########################

ask2: 	my_sem.o ask2.o
		gcc -g my_sem.o ask2.o -o ask2 -lpthread

ask2.o:		ask2.c
			gcc -Wall -g ask2.c -c -lpthread

my_sem.o:	my_sem.c 
			gcc -Wall -g my_sem.c -c 

###################### ask3 ########################

ask3: 	my_sem.o ask3.o
		gcc -g my_sem.o ask3.o -o ask3 -lpthread

ask3.o:		ask3.c
			gcc -Wall -g ask3.c -c -lpthread
			
###################### ask4 ########################

ask4: 	my_sem.o ask4.o
		gcc -g my_sem.o ask4.o -o ask4 -lpthread

ask4.o:		ask4.c
			gcc -Wall -g ask4.c -c -lpthread
