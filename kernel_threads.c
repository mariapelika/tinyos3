#include "kernel_cc.h"
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"


void start_next_thread()
{
    Task call =  CURTHREAD->owner_ptcb->task;
    int argl = CURTHREAD->owner_ptcb->argl;
    void* args = CURTHREAD->owner_ptcb->args;

    CURTHREAD->owner_ptcb->exitval = call(argl,args);

    //CURTHREAD->owner_ptcb->exitfl=1;
    ThreadExit(CURTHREAD->owner_ptcb->exitval);
}

/**
  @brief Create a new thread in the current process.
  */
Tid_t CreateThread(Task task, int argl, void* args)
{	Mutex_Lock(&kernel_mutex);
    PTCB* myptcb=(PTCB *)xmalloc(sizeof(PTCB));//create a temp ptcb

    myptcb->task=task;
    myptcb->isdetached=0;
    myptcb->exitval=-10;//to do
    myptcb->exitfl=0;
    myptcb->argl=argl;
    myptcb->cv=COND_INIT;

    if(task!=NULL){
        myptcb->args=args;
        myptcb->r_counter=0;
        myptcb->tcb= spawn_thread(CURPROC, start_next_thread);

        myptcb->tcb->owner_ptcb=myptcb;
        myptcb->owner=CURPROC;
        myptcb->owner->act_ptcbs++;

        wakeup(myptcb->tcb);
    }

    Mutex_Unlock(&kernel_mutex);
    return (Tid_t)myptcb->tcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t ThreadSelf()
{
    return (Tid_t) CURTHREAD;
}

/**
  @brief Join the given thread.
  */
int ThreadJoin(Tid_t tid, int* exitval)
{
    Mutex_Lock(&kernel_mutex);
    TCB* tcb=(TCB*) tid;
    //fprintf(stderr,"%c",tcb->state);
    //if(tid<0||tid>MAX_THR) {
    //  Mutex_Unlock(& kernel_mutex);
    //return -1;
    //}
    PTCB* called=tcb->owner_ptcb;
    if(get_pid(called->owner)!=GetPid() || ThreadSelf()==tid || called->isdetached==1){ //illegal
        Mutex_Unlock(& kernel_mutex);
        return -1;
    }
    else{
        called->r_counter++;

        while(called->exitfl==0){
            Cond_Wait(& kernel_mutex, & called->cv);
            //break;
        }

        exitval=&(called->exitval);

    }
    Mutex_Unlock(& kernel_mutex);


    return 0;
}

/**
  @brief Detach the given thread.
  */
int ThreadDetach(Tid_t tid)
{
    int return_val;
    Mutex_Lock(& kernel_mutex);
    TCB* tcb=(TCB*) tid;
    PTCB* called=tcb->owner_ptcb;

    if(called->exitfl==1)
        return_val=-1;
    else{
        called->isdetached=1;
        return_val=0;
    }
    Mutex_Unlock(& kernel_mutex);
    return return_val;
}

/**
  @brief Terminate the current thread.
  */
void ThreadExit(int exitval)
{
    Mutex_Lock(& kernel_mutex);
    TCB* tcb2=(TCB*) ThreadSelf();

    PTCB* exited=tcb2->owner_ptcb;

    exited->exitfl=1;
    exited->owner->act_ptcbs--;

    if(exited->owner->act_ptcbs==0){

        if(exited->r_counter>0){   // ksipname auta pou eixan kanei join
            exited->r_counter=0;
            exited->exitval=exitval;
            Cond_Broadcast(& exited->cv);
            //exited->tcb=NULL;//->tcb=NULL;
        }
        Mutex_Unlock(& kernel_mutex);
        Exit(exitval);
        ///free(exited);
    }


    sleep_releasing(EXITED, & kernel_mutex);


}


/**
  @brief Awaken the thread, if it is sleeping.

  This call will set the interrupt flag of the
  thread.

  */
int ThreadInterrupt(Tid_t tid)
{
    return -1;
}


/**
  @brief Return the interrupt flag of the
  current thread.
  */
int ThreadIsInterrupted()
{
    return 0;
}

/**
  @brief Clear the interrupt flag of the
  current thread.
  */
void ThreadClearInterrupt()
{

}