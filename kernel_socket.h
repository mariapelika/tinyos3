#ifndef __KERNEL_SOCKET_H__
#define __KERNEL_SOCKET_H__
#include "tinyos.h"
#include "kernel_pipe.h"
#include "util.h"
void initialize_ports();

typedef enum {
    CONNECTED,
    UNCONNECTED
}connection;

typedef enum { 
    UNPORT,    //se periptwsi UNPORTED 
    PORTED       //epistrefoume success kai socket_type=UNBOUND
}socket_state;


typedef enum {
    BOUND,
    UNBOUND,
    PEER,
    LISTENER
}socket_type;


typedef struct peer_CB{
    pipe_t pipe;
    void * listener; // kratoun pliroforia se poio socket listen
    Pipe_CB *pipe_sock_R;
    Pipe_CB *pipe_sock_W;
    void * conn_peer;
}peer_CB;


typedef struct listen_CB{
    void * accept_sock; /// sock_cb pio geniko
    int request_cnt;
    int acc;
    CondVar conn_cv;
    CondVar acc_cv;
}listen_CB;




typedef struct socket_control_block
{
    Fid_t fid;
    FCB *fcb;
    socket_state state;  /*The state of the socket  */
    socket_type type;
    connection con;
    port_t socket_port; //the port we are bound to MPOREI NA MIN XREIASTEI
    union {
         listen_CB *listens;
         peer_CB *peers;
    }u;
		
}Socket_CB;


#endif




