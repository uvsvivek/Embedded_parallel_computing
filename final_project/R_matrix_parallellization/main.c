/* Epiphany Host Application */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <e-loader.h>  // Comment this line if you are using old SDK ver 5.13.09.10

#include "e-hal.h"

#define FAIL(...) { fprintf(stderr, __VA_ARGS__); exit(1); }
#define SHM_OFFSET 0x01000000
#define cores 3 
//#define col 0x7010
#define rows 4
#define cols 4

typedef struct {
    unsigned int row[2];
    unsigned int col[2];
	int flag[3];
	double q[rows][rows];
	double r[rows][cols];
	int q_flag;
	unsigned long long total_cycles[2];
	} shm_t;



int main()
{
	char *filename = "bin/sample.srec";
	shm_t shm;		/* local copies of shared memory */
	//shm_t oldshm1,shm1;
	e_epiphany_t dev;
	e_mem_t      emem;

	e_set_host_verbosity(H_D0);
	e_set_loader_verbosity(L_D0);


	/* init board, reset epiphany, open workgroup */
	if(e_init(NULL) != E_OK)
		FAIL("Can't init!\n");
	e_reset_system();
	if(e_open(&dev, 0, 0, 4, 4) != E_OK)
		FAIL("Can't open!\n");

	/* initialize, allocate and update shared memory buffer */
	if(e_alloc(&emem, SHM_OFFSET, sizeof(shm_t)) != E_OK)
		FAIL("Can't alloc!\n");
	if(e_write(&emem, 0,0, (off_t)0, &shm, sizeof(shm_t)) == E_ERR)
		FAIL("Can't clear shm!\n");
	
	/* load program */
	if(e_load(filename, &dev, 0,0, E_TRUE) != E_OK)
		FAIL("Can't load!\n");
	
	if(e_load(filename, &dev,0 ,1, E_TRUE) != E_OK)
		FAIL("Can't load!\n");


/*	if(e_load(filename, &dev,0,2,E_TRUE ) != E_OK)
		FAIL("can't load! \n");	*/
	/* =============================================================== */
	
		while(1) {

			/* read shm */
			if(e_read(&emem, 0,0, (off_t)0, &shm,
				sizeof(shm_t)) == E_ERR)
					FAIL("Can't poll!\n");
	printf("shm flag0=%d,shm.flag1=%d\n",shm.flag[0],shm.flag[1]);
			/* compare with previous, break if different */
		if((shm.flag[0]==1) && shm.flag[1]==1 ){// && shm.flag[2]==1){// && (shm.flag[1]==1)){
			break;
		}
	}
	//printf("count=%d",count);
		printf("row:%d %d,    %d %d     \n",shm.row[0],shm.col[0],shm.row[1],shm.col[1]);//,shm.row[2],shm.col[2]);
	
	printf("shm flag0=%d,shm.flag1=%d ,shm.flag3=%d\n",shm.flag[0],shm.flag[1],shm.flag[3]);
	printf("total_cycles 0=%llu ,total_cycles =%llu",shm.total_cycles[0],shm.total_cycles[1]);
	printf("Q Matrix\n");

for(int i=0;i<rows;i++){
		for(int j=0;j<cols;j++){
			printf("%f   ",shm.q[i][j]);
		}
		printf("\n");
	}
	printf("R matrix\n");
	for(int i=0;i<rows;i++){
		for(int j=0;j<cols;j++){
			printf("%f   ",shm.r[i][j]);
		}
		printf("\n");
	}

printf("completeddddd\n");
	/* =============================================================== */



	/* free shm buffer, close workgroup and finalize */
	if(e_free(&emem) != E_OK) FAIL("Can't free!\n");
	if(e_close(&dev) != E_OK) FAIL("Can't close!\n");
	if(e_finalize() != E_OK)  FAIL("Can't finalize!\n");

	return(0);
}
