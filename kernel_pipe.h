#ifndef __KERNEL_PIPE_H
#define __KERNEL_PIPE_H


#include "tinyos.h"
#include "kernel_dev.h"
#include "kernel_streams.h"

typedef struct pipe_control_block {

    //FCB* FID1[MAX_FILEID];  /**< The fileid table for reader */
    //FCB* FID2[MAX_FILEID];  /**< The fileid table for writer */
    char buffer[MAX_BUFF_SIZE];    // buffer for writer and reader
    FCB* fcb1; // for read
    FCB* fcb2; // for writer
    CondVar cv1;
    CondVar cv2;
    int start; //for reader
    int end; // for writer
    int bsize;
} Pipe_CB;

int Pipe_close_R(void * k );
int Pipe_close_W(void * k );

int Pipe_read(void * k, char *buf, unsigned int size);
int Pipe_write(void * k, const char *buf, unsigned int size);
Pipe_CB* pipe_construct(FCB *reader, FCB* writer);
Pipe_CB* Pipe_Sock(pipe_t* pipe_t1, pipe_t *pipe_t2);


#endif



