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
#include <time.h>

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)


struct message1{
    long type;
    int pid;
};

struct message2{ // for msgque3
    long type;
    int pid;
    int page;
};

int main(int argc,char* argv[]){

    if (argc != 4)
    {
        printf("Usage: %s <Message Queue 1 ID> <Message Queue 2 ID> <# Processes>\n", argv[0]);
        exit(1);
    }

    struct sembuf pop = {0, -1, 0};
    struct sembuf vop = {0, 1, 0};

    int msgid1 = atoi(argv[1]);
    int msgid2 = atoi(argv[2]);
    int k = atoi(argv[3]);

    key_t key = ftok("master.c", 100);
    int semid = semget(key, 1, IPC_CREAT | 0666);
    


    while(k>0){

        struct message1 msg1;
        struct message1 msg_mmu;

        msgrcv(msgid1, (void *)&msg1, sizeof(struct message1), 1, 0);

        printf("\t\tScheduling process %d\n", msg1.pid);

        struct message1 msg;
        msg.type= msg1.pid;
        msg.pid=msg1.pid;
        
        msgsnd(msgid1,(void*)&msg,sizeof(struct message1),0);

        msgrcv(msgid2, (void *)&msg_mmu, sizeof(struct message1), 0, 0);

        if (msg_mmu.type == 2)
        {
            printf("\t\tProcess %d terminated\n", msg_mmu.pid);
            k--;
        }
        else if (msg_mmu.type == 1)
        {
            printf("\t\tProcess %d added to end of queue\n", msg_mmu.pid);
            msg1.pid = msg_mmu.pid;
            msg1.type = 1;
            msgsnd(msgid1, (void *)&msg1, sizeof(struct message1), 0);
        }

    }
    printf("\t\tScheduler terminated\n");

    // signal master on semid4
    V(semid);

    return 0;
}
