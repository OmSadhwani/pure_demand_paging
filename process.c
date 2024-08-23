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

struct message1{
    long type;
    int pid;
};

struct message2{
    long type;
    int pid;
    int page;
};
int ct=0;
int printref(int ref[]){
    fprintf(stderr,"Ref str\n");
    for(int i=0;i<ct;i++){
        fprintf(stderr,"%d ",ref[i]);
    }
    fprintf(stderr,"\n");
}

int num[1000];

    char refstr[1000];

int main(int argc,char *argv[]){

    printf("entered\n");

    int msgid1 = atoi(argv[2]);
    int msgid3 = atoi(argv[3]);

    memset(refstr,'\0',sizeof(refstr));
    strcpy(refstr, argv[1]);

    ct=0;

    for(int i=0;i<strlen(refstr);i++){
        int n=0;
        int j=i;
        while(refstr[j]!='.' && refstr[i]!='\0'){
            n=n*10+(refstr[j]-'0');
            j++;
            
        }
        i=j;
            num[ct]=n;
            ct++;

    }

    // char temprefstr[1000];


    printf("Process has started\n");
    // fprintf(stderr,"Reference string = %s and pid = %d\n",refstr,getpid());
    

    int pid = getpid();

    struct message1 msg1;
    msg1.type = 1;
    msg1.pid = pid;

    // send pid to ready queue
    msgsnd(msgid1, (void *)&msg1, sizeof(struct message1), 0);
    struct message1 msg;
    if (msgrcv(msgid1, &msg, sizeof(struct message1), pid, 0) == -1) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }

    int i=0;
    while(i<ct){
        
        // fprintf(stderr,"ref string = %s\n",refstr);
        printref(num);
        // fprintf(stderr,"i = %d\n",num[i]);
        // int num=0;
        // int prev_i= i;
        // while(refstr[i]!='.'){
        //     num=num*10+(refstr[i]-'0');
        //     i++;
        // }
        // fprintf(stderr,"ref string = %s\n",refstr);
        // printref(refstr);
        // i++;
        // fprintf(stderr,"i = %d\n",i);

        struct message2 msg2;
        msg2.page=num[i];
        msg2.type=1;
        msg2.pid=pid;
        msgsnd(msgid3, (void *)&msg2, sizeof(struct message2), 0);

        msgrcv(msgid3, (void *)&msg2, sizeof(struct message2), pid, 0);
        printf("msg receivedddd\n");
        if (msg2.page == -2)
        {
            fprintf(stderr,"Process %d: ", pid);
            fprintf(stderr,"Illegal Page Number\nTerminating\n");
            exit(1);
        }
        else if (msg2.page == -1)
        {
            // fprintf(stderr,"Process %d: ", pid);
            // fprintf(stderr,"Page Fault\nWaiting for page to be loaded\n");
            // wait for the page to be loaded
            // scheduler will signal when the page is loaded
            // i=prev_i;
            
            struct message1 msg;

            // fprintf(stderr,"before recv ref string = %s\n",refstr);
            printref(num);
            
            if (msgrcv(msgid1, &msg, sizeof(struct message1), pid, 0) == -1) {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
            // fprintf(stderr,"after recv ref string = %s\n",refstr);
            // strcpy(refstr,temprefstr);
            fprintf(stderr,"after recv\n");
            printref(num);

            continue;
        }
        else
        {
            fprintf(stderr,"Process %d: ", pid);
            fprintf(stderr,"Frame %d allocated\n", msg2.page);
            i++;
            // strcpy(refstr,temprefstr);
        }
    }

    printf("Process %d: ", pid);
    printf("Got all frames properly, terminating\n");
    struct message2 msg2;
    msg2.type=1;
    msg2.pid=pid;
    msg2.page=-9;

    msgsnd(msgid3, (void *)&msg2, sizeof(struct message2), 0);
    // fprintf(stderr,"Terminating\n");

    return 0;



}