#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <signal.h>

#define PROB 0.01

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

int pidscheduler;
int pidmmu;
int msgid1,msgid2,msgid3,shmid1,shmid2,semid;
int *sm2;


typedef struct SM1
{
    int pid;         // process id
    int mi;          // number of required page
    int pagetable[25][3]; // page table
    int totalpagefaults;
    int totalillegalaccess;
} SM1;

SM1 *sm1;

void sighand(int signum)
{
    if(signum == SIGINT)
    {
        // kill scheduler,mmu and all the processes
        kill(pidscheduler,SIGINT);
        kill(pidmmu,SIGINT);

        shmdt(sm1);
        shmdt(sm2);
       
     
        shmctl(shmid1, IPC_RMID,0);

       
        shmctl(shmid2, IPC_RMID, 0);

        
        
        // remove semaphores
        semctl(semid, 0, IPC_RMID, 0);
        

        // remove message queues
        msgctl(msgid1, IPC_RMID, NULL);
        msgctl(msgid2, IPC_RMID, NULL);
        msgctl(msgid3, IPC_RMID, NULL);
        exit(1);
    }
}

int getRandom(int min, int max) {
    return rand() % (max - min + 1) + min;
}

int main(){
    signal(SIGINT,sighand);
    srand(time(0));


    struct sembuf pop = {0, -1, 0};
    struct sembuf vop = {0, 1, 0};

    int k, m, f;
    printf("Enter the number of processes: ");
    scanf("%d", &k);
    printf("Enter the Virtual Address Space size: ");
    scanf("%d", &m);
    printf("Enter the Physical Address Space size: ");
    scanf("%d", &f);

    key_t key = IPC_PRIVATE;
    shmid1 = shmget(key, k * sizeof(SM1), IPC_CREAT | 0666);
    sm1 = (SM1 *)shmat(shmid1, NULL, 0);

    for(int i=0;i<k;i++){
        sm1[i].totalpagefaults = 0;
        sm1[i].totalillegalaccess = 0;
    }
    key =IPC_PRIVATE;
    shmid2 = shmget(key, (f + 1) * sizeof(int), IPC_CREAT | 0666);
    sm2 = (int *)shmat(shmid2, NULL, 0);

    for(int i=0;i<f;i++){
        sm2[i]=1;
    }
    sm2[f]=-1;

    key = ftok("master.c", 100);
    semid = semget(key, 1, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, 0);

    // Message Queue 1 for Ready Queue
    key = IPC_PRIVATE;
    msgid1 = msgget(key, IPC_CREAT | 0666);

    // Message Queue 2 for Scheduler and mmu
    key = IPC_PRIVATE;
    msgid2 = msgget(key, IPC_CREAT | 0666);
    // printf("%d\n",msgid2);

    // Message Queue 3 for Memory Management Unit and processes
    key = IPC_PRIVATE;
    msgid3 = msgget(key, IPC_CREAT | 0666);

    // convert msgid to string
    char msgid1str[20], msgid2str[20], msgid3str[20];
    sprintf(msgid1str, "%d", msgid1);
    sprintf(msgid2str, "%d", msgid2);
    sprintf(msgid3str, "%d", msgid3);

    // convert shmids to string
    char shmid1str[20], shmid2str[20];
    sprintf(shmid1str, "%d", shmid1);
    sprintf(shmid2str, "%d", shmid2);

    char strk[20];
    sprintf(strk, "%d", k);
   
    char refstr[k][1000];

    pidscheduler= fork();
    if(pidscheduler==0){
         execlp("./sched", "./sched", msgid1str, msgid2str, strk, NULL);
    }

    // usleep(10);

    pidmmu= fork();
    if(pidmmu==0){
        execlp("xterm", "xterm", "-T", "Memory Management Unit", "-e", "./mmu", msgid2str, msgid3str, shmid1str, shmid2str, NULL);
        // execlp("./mmu","./mmu", msgid2str, msgid3str, shmid1str, shmid2str, NULL);
    }

    printf("here\n");

    for(int i=0;i<k;i++){
        sm1[i].mi=rand()%m+1;
        for(int j=0;j<25;j++){
            sm1[i].pagetable[j][0]=-1;
            sm1[i].pagetable[j][1]=0;
        }
        sm1[i].totalillegalaccess=0;
        sm1[i].totalpagefaults=0;

        int len= getRandom(2*sm1[i].mi,10*sm1[i].mi);
        memset(refstr[i],'\0',sizeof(refstr[i]));

        for(int j=0;j<len;j++){

            int num= rand()%sm1[i].mi;
            if ((double)rand() / RAND_MAX < PROB) {
                num=getRandom(sm1[i].mi,m); //random number between mi and m
            }
            char temp[20];
            memset(temp,'\0',sizeof(temp));
            sprintf(temp,"%d.",num);
            strcat(refstr[i],temp);
        }

    }
    for(int i=0;i<k;i++){
        usleep(250000);
        int pid= fork();
        if(pid!=0){
            sm1[i].pid=pid;
        }
        else{
            sm1[i].pid= getpid();
            execlp("./process", "./process", refstr[i], msgid1str, msgid3str, NULL);
        }
    }

    P(semid);
    FILE* file = fopen("result.txt","a+");

    int total_fault = 0;
    int total_ill = 0;

    for(int l=0;l<k;l++){
        fprintf(file,"the total pagefaults in this process %d is %d and total illegal access = %d \n",l+1,sm1[l].totalpagefaults,sm1[l].totalillegalaccess);
        fprintf(file,"--------------------------------------\n");
        fflush(file);
        total_fault+=sm1[l].totalpagefaults;
        total_ill+=sm1[l].totalillegalaccess;
    }

    fprintf(file,"Total Page Fault = %d\n",total_fault);
    fprintf(file,"Total Illegal Accesses = %d\n",total_ill);
    fflush(file);
    fclose(file);

    // sleep(100);

    // terminate Scheduler
    kill(pidscheduler, SIGINT);

    // terminate Memory Management Unit
    kill(pidmmu, SIGINT);

    // detach and remove shared memory
    shmdt(sm1);
    shmctl(shmid1, IPC_RMID, NULL);

    shmdt(sm2);
    shmctl(shmid2, IPC_RMID, NULL);



    
    semctl(semid, 0, IPC_RMID, 0);
    

    msgctl(msgid1, IPC_RMID, NULL);
    msgctl(msgid2, IPC_RMID, NULL);
    msgctl(msgid3, IPC_RMID, NULL);

    return 0;




}