#include "kernel_socket.h"
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_cc.h"
#include "kernel_pipe.h"

Fid_t PORTS_TABLE[MAX_PORT];


void initialize_ports()
{
    for(int i=0;i<MAX_PORT;i++) {
        PORTS_TABLE[i]=-1;
    }
}


int peer_close(void *k){
    Socket_CB *temp=(Socket_CB*)k;
    temp->fcb=NULL;
    //free(temp->pipe_sock);
    //free(temp);
    return 0;
}

int listen_close(void *k){

    Socket_CB *temp=(Socket_CB*)k;
    PORTS_TABLE[temp->socket_port]=-1;
    ///Cond_Broadcast(&temp->u.listens->lis_cv);
    ///temp->type=UNBOUND;


    //Mutex_Lock(&kernel_mutex);
    //Cond_Broadcast(&temp->u.listens->acc_cv);
    //Mutex_Unlock(&kernel_mutex);
    temp->fcb=NULL;
    ///free(temp->pipe_sock);
    ///free(temp);
    return 0;
}

int fake_read1(void * k, char *buf, unsigned int size){
    return -1;}

int fake_write1(void * l,const char *buf, unsigned int size){
    return -1;}

int peer_write(void * k, const char *buf, unsigned int size) {

    Socket_CB* temp_sock = (Socket_CB *)k;
    if(temp_sock->type!=PEER) return -1;


    int a= Pipe_write(temp_sock->u.peers->pipe_sock_W, buf,size);
    return a;
}

int peer_read(void * k, char *buf, unsigned int size) {
    Socket_CB* temp_sock = (Socket_CB *)k;

    if(temp_sock->type!=PEER) return -1;

    int a= Pipe_read(temp_sock->u.peers->pipe_sock_R, buf,size);
    return a;
}


file_ops listener_ops={
        .Open=NULL,
        .Read=fake_read1,
        .Write=fake_write1,
        .Close=listen_close
};

file_ops peer_ops={
        .Open=NULL,
        .Read=peer_read,
        .Write=peer_write,
        .Close=peer_close
};
/*Return a new socket bound on a port.

This function returns a file descriptor for a new socket object. If the port argument is NOPORT, then the socket will not be bound to a port. Else, the socket will be bound to the specified port.
----------------
Returns
    a file id for the new socket, or NOFILE on error. Possible reasons for error:

        the port is iilegal
        the available file ids for the process are exhausted

*/


Fid_t Socket(port_t port)
{
    if(port>MAX_PORT || port<NOPORT){
        return NOFILE;
    }
    /*First we create the Socket_CB */
    Socket_CB* new_socket=(Socket_CB *)xmalloc(sizeof(Socket_CB));
    //tha kanoume malloc listener/peer meta se alli sinartisi
    int s1=FCB_reserve(1,&new_socket->fid,&new_socket->fcb);
    if(s1!=1) return NOFILE;

    new_socket->fcb->streamfunc=&listener_ops;
    new_socket->fcb->streamobj=new_socket;
    if(port==NOPORT) {// do not bound but return
        /// new_socket->u.peers->pipe_sock=NULL;
        new_socket->state=UNPORT;
        new_socket->type=UNBOUND;
        new_socket->socket_port =0;
        new_socket->con=UNCONNECTED;
        return new_socket->fid;
    }
    else {
        /// new_socket->u.peers->pipe_sock=NULL;
        new_socket->state=PORTED;
        new_socket->type=BOUND;
        new_socket->socket_port = port;
        new_socket->con=UNCONNECTED;
        return new_socket->fid;
    }
}


/*Initialize a socket as a listening socket.

A listening socket is one which can be passed as an argument to Accept. Once a socket becomes a listening socket, it is not possible to call any other functions on it except Accept, and Dup2().

The socket must be bound to a port, as a result of calling Socket. On each port there must be a unique listening socket (although any number of non-listening sockets are allowed).

Returns
    0 on success, -1 on error. Possible reasons for error:

        the file id is not legal
        the socket is not bound to a port
        the port bound to the socket is occupied by another listener  SOS
        the socket has already been initialized*/

//allaksa filecontrol block prosthesa flag

int Listen(Fid_t sock)
{
    if(get_fcb(sock)==NULL || sock<0 || sock>MAX_FILEID) return -1;
    FCB *temp=get_fcb(sock);//fcb pou exei to socket;

    if(temp->streamobj==NULL) return -1;

    Socket_CB *t_socket;//=(Socket_CB*)xmalloc(sizeof(Socket_CB));
    t_socket=temp->streamobj;
    if(t_socket->type!=BOUND) return -1;

    if(PORTS_TABLE[t_socket->socket_port]!=-1){
        return -1;
    }


    t_socket->u.listens=(listen_CB*)xmalloc(sizeof(listen_CB)); //allocate mem
    PORTS_TABLE[t_socket->socket_port]=sock;
    t_socket->type=LISTENER;
    t_socket->u.listens->request_cnt=0;
    t_socket->u.listens->acc_cv=COND_INIT;
    t_socket->u.listens->conn_cv=COND_INIT;
    t_socket->u.listens->acc=0;
    //logw tou flag=LISTENER parakatw tha elegxoume ean mporei na kanei Accept,Dup2 ktl

    return 0;
}


Fid_t Accept(Fid_t lsock)
{
    if(get_fcb(lsock)==NULL || lsock<0 || lsock>MAX_FILEID) return NOFILE;
    FCB *temp=get_fcb(lsock);//fcb pou exei to socket;
    if(temp->streamobj==NULL) return NOFILE;
    Socket_CB *listener;//=(Socket_CB*)xmalloc(sizeof(Socket_CB));
    listener=temp->streamobj;
    if(listener->type!=LISTENER) {
        return NOFILE;

    }
    listener->u.listens->request_cnt++;

    while(listener->u.listens->acc==0) {
        Mutex_Lock(&kernel_mutex);

        if(get_fcb(lsock)==NULL) {
            listener->u.listens->request_cnt--;
            break;
            return NOFILE;
        }
        Cond_Signal(&listener->u.listens->conn_cv);
        Cond_Wait(&kernel_mutex, &listener->u.listens->acc_cv);
        Mutex_Unlock(&kernel_mutex);

    }


    if(get_fcb(lsock)==NULL) {
        listener->u.listens->request_cnt--;
        return NOFILE;
    }


    Fid_t sock1 =Socket(listener->socket_port);
    if(sock1==NOFILE) return NOFILE;

    /// elegxos an uparxei Socket
    FCB *temp1=get_fcb(sock1);
    Socket_CB * new_socket;///=(Socket_CB*)xmalloc(sizeof(Socket_CB));
    new_socket=temp1->streamobj;

    new_socket->u.peers=(peer_CB*)xmalloc(sizeof(peer_CB)); //allocate mem

    new_socket->con=CONNECTED;
    new_socket->type=PEER;
    new_socket->fcb->streamfunc=&peer_ops;

    listener->u.listens->accept_sock=new_socket;
    new_socket->u.peers->listener=listener;
    Mutex_Lock(&kernel_mutex);
    Cond_Signal(&listener->u.listens->conn_cv);
    Mutex_Unlock(&kernel_mutex);


    return sock1;
}


int Connect(Fid_t sock, port_t port, timeout_t timeout) {
    if (get_fcb(sock) == NULL || sock < 0 || sock > MAX_FILEID){
        return -1;
    }
    FCB *temp = get_fcb(sock);//fcb pou exei to socket;
    Socket_CB *connect_sock;///=(Socket_CB*)xmalloc(sizeof(Socket_CB));
    connect_sock = temp->streamobj;

    if (connect_sock->type != BOUND && connect_sock->type != UNBOUND) { //AN DEN EINAI BOUND I UNBOUND
        return -1;
    }
    if (connect_sock->con == CONNECTED) {
        return -1;
    }
    if (port > MAX_PORT || port < NOPORT) {
        return -1;
    }
    if (PORTS_TABLE[port] == -1) {
        return -1;
    }
    FCB *fcb1 = get_fcb(PORTS_TABLE[port]);//fcb pou exei to socket;
    Socket_CB *listener;///=(Socket_CB*)xmalloc(sizeof(Socket_CB));
    listener = fcb1->streamobj;


    if(listener->u.listens->request_cnt==0) {
        fprintf(stderr,"\n %d if\n",listener->u.listens->request_cnt);
        return -1;
    }


    connect_sock->u.peers=(peer_CB*)xmalloc(sizeof(peer_CB)); //allocate mem


    /// gia na vroume to listener tou port kai na ton ksipnisoume

    if(listener->u.listens->request_cnt>0 && listener->u.listens->acc==0 ) {
        listener->u.listens->acc=1;

        Mutex_Lock(&kernel_mutex);
        Cond_Signal(&listener->u.listens->acc_cv);
        Cond_Wait(&kernel_mutex, &listener->u.listens->conn_cv);
        Mutex_Unlock(&kernel_mutex);
    }
    Socket_CB *acc_sock = (Socket_CB *) listener->u.listens->accept_sock;

    if (acc_sock != NULL) {
        connect_sock->type = PEER;

        connect_sock->con = CONNECTED;
        connect_sock->u.peers->listener=listener;
        connect_sock->fcb->streamfunc = &peer_ops;
        pipe_t pipe_t1;
        pipe_t pipe_t2;
        Pipe_CB *pipe;

        pipe = Pipe_Sock(&pipe_t1,&pipe_t2);
        if (pipe == NULL ) {
            return -1;
        }

        connect_sock->u.peers->pipe_sock_R=&pipe[0];
        connect_sock->u.peers->pipe_sock_W=&pipe[1];

        acc_sock->u.peers->pipe_sock_R=&pipe[1];
        acc_sock->u.peers->pipe_sock_W=&pipe[0];

        connect_sock->u.peers->conn_peer = acc_sock;
        acc_sock->u.peers->conn_peer = connect_sock;

        listener->u.listens->request_cnt--;
        //listener->u.listens->acc=0;

    }
    else{
        return -1;
    }

    return 0;
}


/*Shut down one direction of socket communication.

With a socket which is connected to another socket, this call will shut down one or the other direction of communication. The shut down of a direction has implications similar to those of a pipe's end shutdown. More specifically, assume that this end is socket A, connected to socket B at the other end. Then,

    if ShutDown(A, SHUTDOWN_READ) is called, any attempt to call Write(B,...) will fail with a code of -1.
    if ShutDown(A, SHUTDOWN_WRITE)is called, any attempt to callRead(B,...)` will first exhaust the buffered data and then will return 0.
    if ShutDown(A, SHUTDOWN_BOTH)` is called, it is equivalent to shutting down both read and write.

After shutdown of socket A, the corresponding operation Read(A,...) or Write(A,...) will return -1.

Shutting down multiple times is not an error.

Parameters
    sock	the file ID of the socket to shut down.
    how	the type of shutdown requested

Returns
    0 on success and -1 on error. Possible reasons for error:

        the file id sock is not legal (a connected socket stream).

*/
int ShutDown(Fid_t sock, shutdown_mode how)
{
    if (get_fcb(sock) == NULL || sock < 0 || sock > MAX_FILEID) return -1;


    FCB *temp = get_fcb(sock);//fcb pou exei to socket;
    Socket_CB *socket;
    socket = temp->streamobj;

    if(socket->type==PEER)
    {


        if(how==SHUTDOWN_READ)
        {
            Pipe_close_R(socket->u.peers->pipe_sock_R->fcb1);
            Pipe_close_W(socket->u.peers->pipe_sock_R->fcb2);
        }
        else if(how==SHUTDOWN_WRITE)
        {
            Pipe_close_W(socket->u.peers->pipe_sock_W->fcb2);
        }
        else if(how==SHUTDOWN_BOTH)
        {
            Pipe_close_R(socket->u.peers->pipe_sock_R->fcb1);
            Pipe_close_W(socket->u.peers->pipe_sock_W->fcb2);
            Pipe_close_R(socket->u.peers->pipe_sock_R->fcb1);
            Pipe_close_W(socket->u.peers->pipe_sock_W->fcb2);
        }
        return 0;
    }
    else return -1;

}
