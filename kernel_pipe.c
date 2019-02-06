#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"
#include "kernel_cc.h"

//#include "kernel_pipe.h"
/*OPERATIONS FOR READER*/

int Pipe_read(void * k, char *buf, unsigned int size) {

    Pipe_CB *temp_pipe = (Pipe_CB *)k;

    if (temp_pipe->bsize == 0 && temp_pipe->fcb2!=NULL) {
        Mutex_Lock(&kernel_mutex);
        Cond_Signal(&temp_pipe->cv2);
        Cond_Wait(&kernel_mutex,&temp_pipe->cv1);
        Mutex_Unlock(&kernel_mutex);
    }

    int r_size;
    int max_size;
    if (temp_pipe->start >= temp_pipe->end)
        max_size = MAX_BUFF_SIZE - temp_pipe->start + temp_pipe->end;
    else
        max_size = temp_pipe->end - temp_pipe->start;
    if (size > MAX_BUFF_SIZE){
        r_size = MAX_BUFF_SIZE;
    }
    else
        r_size = size;
    if (r_size > max_size) {
        r_size = max_size;
    }

    if (temp_pipe->bsize == 0 && temp_pipe->fcb2==NULL) {//EOF
        return 0;
    }

    for (int i = 0; i < r_size; i++) {
        buf[i] = temp_pipe->buffer[(temp_pipe->start + i) % MAX_BUFF_SIZE];
    }
    temp_pipe->start = (temp_pipe->start + r_size) % MAX_BUFF_SIZE;
    temp_pipe->bsize = temp_pipe->bsize - r_size;
    Cond_Signal(&temp_pipe->cv2);
    return r_size;
}

int Pipe_write(void * k, const char *buf, unsigned int size) {

    Pipe_CB *temp_pipe = (Pipe_CB *)k;
    if(temp_pipe->fcb1==NULL) { // elegxos an exei kleisei o reader
        return -1;
    }
    int r_size;
    int max_size;
    if (temp_pipe->start > temp_pipe->end)
        max_size =temp_pipe->start - temp_pipe->end;
    else
        max_size = MAX_BUFF_SIZE - temp_pipe->end + temp_pipe->start;
    if (size > MAX_BUFF_SIZE) {
        r_size = MAX_BUFF_SIZE;   ///koimizw
    }
    else {
        r_size = size;
    }
    if (r_size > max_size) { ///koimizw
        r_size = max_size;
        Mutex_Lock(&kernel_mutex);
        Cond_Signal(&temp_pipe->cv1);
        Cond_Wait(&kernel_mutex, &temp_pipe->cv2);
        Mutex_Unlock(&kernel_mutex);
        r_size=size;
    }
    if (temp_pipe->bsize == MAX_BUFF_SIZE)//
        return 0;
    for (int i = 0; i < r_size; i++) {
        temp_pipe->buffer[(temp_pipe->end+i)% MAX_BUFF_SIZE]=buf[i];
    }
    temp_pipe->end = (temp_pipe->end + r_size) % MAX_BUFF_SIZE;
    temp_pipe->bsize = temp_pipe->bsize + r_size;
    Cond_Signal(&temp_pipe->cv1);
    return r_size;
}

int Pipe_close_R(void * k ){
    Pipe_CB *temp_pipe = (Pipe_CB *)k;
    //temp_pipe->start=0;
    //temp_pipe->end=0;
    temp_pipe->fcb1=NULL;
    return 0;
}

int Pipe_close_W(void * k ){
    Pipe_CB *temp_pipe = (Pipe_CB *)k;
    temp_pipe->fcb2=NULL;

    return 0;
}

int fake_read(void * k, char *buf, unsigned int size){
    return -1;}

int fake_write(void * l,const char *buf, unsigned int size){
    return -1;}

file_ops _read_ops={
        .Open=NULL,
        .Read=Pipe_read,  //prepei na ilopoiithei
        .Write=fake_write,
        .Close=Pipe_close_R
};


/*OPERATIONS FOR WRITER*/

file_ops _write_ops={
        .Open=NULL,
        .Read=fake_read,//prepei na ilopoiithei
        .Write=Pipe_write,
        .Close=Pipe_close_W
};


Pipe_CB* pipe_construct(FCB *reader, FCB* writer)
{
    Pipe_CB *pipe1=(Pipe_CB *)xmalloc(sizeof(Pipe_CB));
    pipe1->cv1=COND_INIT;
    pipe1->cv2=COND_INIT;
    pipe1->start=0;
    pipe1->end=0;
    reader->streamobj=pipe1;
    writer->streamobj=pipe1;
    pipe1->fcb1=reader;
    pipe1->fcb2=writer;

    return pipe1;

}
int Pipe(pipe_t* pipe)
{
    FCB * fcb_array[2];
    Fid_t fid[2];

    int s1=FCB_reserve(2,fid,fcb_array);
    if(s1==0)  return -1;
    pipe_construct(fcb_array[0],fcb_array[1]);
    pipe->read=fid[0];
    pipe->write=fid[1];

    fcb_array[0]->streamfunc=&_read_ops;
    fcb_array[1]->streamfunc=&_write_ops;
    return 0;
}

Pipe_CB* Pipe_Sock(pipe_t* pipe_t1, pipe_t* pipe_t2)
{
    Pipe_CB *pipe1=(Pipe_CB *)xmalloc(2*sizeof(Pipe_CB));
    pipe1[0].cv1=COND_INIT;
    pipe1[0].cv2=COND_INIT;
    pipe1[0].start=0;
    pipe1[0].end=0;

    pipe1[1].cv1=COND_INIT;
    pipe1[1].cv2=COND_INIT;
    pipe1[1].start=0;
    pipe1[1].end=0;

    FCB * fcb_array[4];
    Fid_t fid[4];

    int s1=FCB_reserve(4,fid,fcb_array);
    if(s1==0)  return NULL;

    pipe_t1->read=fid[0];
    pipe_t1->write=fid[1];

    fcb_array[0]->streamobj=&pipe1[0];
    fcb_array[1]->streamobj=&pipe1[0];
    fcb_array[0]->streamfunc=&_read_ops;
    fcb_array[1]->streamfunc=&_write_ops;

    fcb_array[2]->streamobj=&pipe1[1];
    fcb_array[3]->streamobj=&pipe1[1];
    fcb_array[2]->streamfunc=&_read_ops;
    fcb_array[3]->streamfunc=&_write_ops;

    pipe1[0].fcb1=fcb_array[0];
    pipe1[0].fcb2=fcb_array[1];

    pipe1[1].fcb1=fcb_array[2];
    pipe1[1].fcb2=fcb_array[3];
    return pipe1;
}

