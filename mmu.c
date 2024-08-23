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
#include <limits.h>


typedef struct SM1
{
    int pid;         // process id
    int mi;          // number of required page
    int pagetable[25][3]; // page table
    int totalpagefaults;
    int totalillegalaccess;
} SM1;

struct message1
{
    long type;
    int pid;
};

struct message2{
    long type;
    int pid;
    int page;
};

int main(int argc, char* argv[]){
    if (argc != 5)
    {
        printf("Usage: %s <Message Queue 2 ID> <Message Queue 3 ID> <Shared Memory 1 ID> <Shared Memory 2 ID>\n", argv[0]);
        exit(1);
    }

    FILE* file = fopen("result.txt","w");

    int msgid2 = atoi(argv[1]);
    int msgid3 = atoi(argv[2]);
    int shmid1 = atoi(argv[3]);
    int shmid2 = atoi(argv[4]);
    int timestamp=0;

    SM1 *sm1 = (SM1 *)shmat(shmid1, NULL, 0);
    int *sm2 = (int *)shmat(shmid2, NULL, 0);
        struct message2 msg;
            struct message1 msg1;

    while(1){
        timestamp++;
        msgrcv(msgid3, (void *)&msg, sizeof(struct message2), 1, 0);

        printf("Global Ordering - (Timestamp %d, Process %d, Page %d)\n", timestamp, msg.pid, msg.page);
        fprintf(file,"Global Ordering - (Timestamp %d, Process %d, Page %d)\n", timestamp, msg.pid, msg.page);
        fflush(file);

        int i = 0;
        while (sm1[i].pid != msg.pid)
        {
            i++;
        }
        // printf("iiiiiiiiiiiiiiiiiiiiii %d\n",i);

        int page = msg.page;
        if (page == -9)
        {
            // process is done
            // free the frames
            for (int j = 0; j < 25; j++)
            {   
                if(sm1[i].pagetable[j][0]!=-1){
                    sm2[sm1[i].pagetable[j][0]]=1;
                }
                sm1[i].pagetable[j][0]=-1;
                sm1[i].pagetable[j][1]=0;
                sm1[i].pagetable[j][2]=INT_MAX;
            }
            msg1.type = 2;
            msg1.pid = msg.pid;
            msgsnd(msgid2, (void *)&msg1, sizeof(struct message1), 0);
        }
        else if (sm1[i].pagetable[page][0] != -1 )
        {
            // page there in memory and valid, return frame number
            sm1[i].pagetable[page][2] = timestamp;
            struct message2 msg3;
            msg3.page = sm1[i].pagetable[page][0];
            msg3.pid=sm1[i].pid;
            msg3.type=sm1[i].pid;
            msgsnd(msgid3, (void *)&msg3, sizeof(struct message2), 0);
        }
        else if (page >= sm1[i].mi)
        {
            // illegal page number
            // ask process to kill themselves
            // struct message2 msg3;
            sm1[i].totalillegalaccess++;
            msg.page = -2;
            msg.pid=sm1[i].pid;
            msg.type=sm1[i].pid;

            msgsnd(msgid3, (void *)&msg, sizeof(struct message2), 0);

            // update total illegal access
            // sm1[i].totalillegalaccess++;

            printf("Invalid Page Reference - (Process %d, Page %d)\n", i + 1, page);
            fprintf(file,"Invalid Page Reference - (Process %d, Page %d)\n", i + 1, page);
            fflush(file);

            for (int j = 0; j < 25; j++)
            {
                if(sm1[i].pagetable[j][0]!=-1){
                    sm2[sm1[i].pagetable[j][0]]=1;
                }
                sm1[i].pagetable[j][0]=-1;
                sm1[i].pagetable[j][1]=0;
                sm1[i].pagetable[j][2]=INT_MAX;
            }
            struct message1 msg1;
            msg1.type = 2;
            msg1.pid = msg.pid;
            msgsnd(msgid2, (void *)&msg1, sizeof(struct message1), 0);
        }
        else{
            // page fault
            sm1[i].totalpagefaults++;
            struct message2 msg3;
            msg3.page = -1;
            msg3.pid=sm1[i].pid;
            msg3.type=sm1[i].pid;

            msgsnd(msgid3, (void *)&msg3, sizeof(struct message2), 0);
            printf("Page fault sequence - (Process %d, Page %d)\n", i + 1, page);
            fprintf(file,"Page fault sequence - (Process %d, Page %d)\n", i + 1, page);
            fflush(file);
            // sm1[i].totalpagefaults++;

            int j = 0;
            // printf("Here\n");
            while(sm2[j]!=-1){
                if(sm2[j]==1){
                    sm2[j]=0;
                    break;
                }
                j++;
            }
            // printf("Here\n");
            if(sm2[j]!=-1){
                sm1[i].pagetable[page][0] = j;
                sm1[i].pagetable[page][1] = 1;
                sm1[i].pagetable[page][2] = timestamp;
                struct message1 mess;
                mess.type = 1;
                mess.pid = msg.pid;
                
                msgsnd(msgid2, (void *)&mess, sizeof(struct message1), 0);
                printf("Message sent\n");
            }
            else{
                // lru cache
                int min = INT_MAX;
                int minpage = -1;
                for (int k = 0; k < sm1[i].mi; k++)
                {
                    if (sm1[i].pagetable[k][2] < min)
                    {
                        min = sm1[i].pagetable[k][2];
                        minpage = k;
                    }
                }

                
                // sm1[i].pagetable[minpage][1] = 0;
                if(minpage!=-1){
                    sm1[i].pagetable[page][0] = sm1[i].pagetable[minpage][0];
                    sm1[i].pagetable[minpage][0]=-1;
                    sm1[i].pagetable[page][1] = 1;
                    sm1[i].pagetable[page][2] = timestamp;
                    sm1[i].pagetable[minpage][2] = INT_MAX;
                }

                struct message1 msg2;
                msg2.type = 1;
                msg2.pid = msg.pid;
                msgsnd(msgid2, (void *)&msg2, sizeof(struct message1), 0);
            }
            

        }




    }



}
