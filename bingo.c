#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <time.h>

#include <signal.h>
#include "csapp.h"

void *thread(void *vargp);
void game(int fd);
void sendError(int fd, char *cause, char *longmsg);

pthread_rwlock_t lock;

int count = 3;                  // total user count - starts from 3 pseudo players
int order[5];                   // play from order[0] to order[4], stores player array index
int checkOrder[5];              // to check if the client is already in the order

int clientAGotVal = 0;          // check if client A got numbers
int clientBGotVal = 0;          // check if client B got numbers
int clientsGotVal = 0;          // check if client A, B got numbers

int currentIndex = 0;           //current player by the index of the order array

int secretVal = 0;              // culprit's message

int finished = 0;               // 1 if game finished
int winner[5];                  // winners' according to the index of the players array

//player information struct
struct player {
    char code[9];               // user security code
    int num[25];                // bingo array
    int check[100];             // check duplications at number generation - during game : value -> (num array index+1) matching
    int checkGot[25];           // check duplicated selection during the game
    int index;                  // 1 : Main culprit, 2 : Co-partner, others : pseudo players - index for players
} players[5];                   // 0 : Main culprit, 1 : Co-partner, others : pseudo players

//last five values that are chosen from clients
int lastFive[5];
//index for the above array
int indexFive = 0;

//check horizontal bingo
int checkHorizontal(int currPlayer, int row) {
    int *cG = players[currPlayer].checkGot;
    if(cG[5*row+0] && cG[5*row+1] && cG[5*row+2] && cG[5*row+3] && cG[5*row+4])
        return 1;
    return 0;
}

//check vertical bingo
int checkVertical(int currPlayer, int col) {
    int *cG = players[currPlayer].checkGot;
    if(cG[col+0*5] && cG[col+1*5] && cG[col+2*5] && cG[col+3*5] && cG[col+4*5])
        return 1;
    return 0;

}

//check diagonal bingo
int checkDiagonal(int currPlayer) {
    int *cG = players[currPlayer].checkGot;
    if(cG[0] && cG[6] && cG[12] && cG[18] && cG[24]) 
        return 1;
    else if(cG[4] && cG[8] && cG[12] && cG[16] && cG[20]) 
        return 1;
    return 0;
}

//check bingo using above functions
int checkBingo(int currPlayer) {
    int i;
    for(i=0; i<5; i++) {
        if(checkHorizontal(currPlayer, i) || checkVertical(currPlayer, i))
            return 1;
    }
    if(checkDiagonal(currPlayer))
        return 1;

    return 0;
}

//check if the game is finished using above functions
int checkFinished() {
    
    int i;

    for(i=0; i<5; i++) {
        if(checkBingo(i)) {
            winner[i] = 1;
            finished = 1;
        }
    }
    if(finished)
        printf("Game Ended\n");
    return finished;
}

void store(int value) { // store last 5 values for the right rendering
    lastFive[indexFive++] = value;
    if(indexFive == 5)
        indexFive = 0;
}

//apply the changed state of the number to all clients' matrices
void checkValuesForAll(int value) {
    int i, j;
    for(j=0; j<5; j++) {
        for(i=0;i<25;i++) {
            if(players[j].num[i] == value)
                break;
        }
        players[j].checkGot[i] = 1;
    }
    store(value);
}

//process the current state where the designated player picked the value
void play(int currPlayer, int value) {
    int temp;
    int currentPlayer = currPlayer; // current player by the index of the players array

    printf("currPlayer : %d\n", currentPlayer);

    if(currentPlayer == 0 || currentPlayer == 1) {
       
        checkValuesForAll(value);

        if(checkFinished()) {
            return;
        }

        if(currentIndex == 4) {
            currentIndex = 0;
        }
        else
            currentIndex++;

        currentPlayer = order[currentIndex];
    }

    //for pseudo players
    while(currentPlayer != 0 && currentPlayer != 1) {
        sleep(3);

        temp = rand()%25;
        while(players[currentPlayer].checkGot[temp]) {
            temp = rand()%25;
        }

        printf("currentPlayer : %d, num : %d\n", currentPlayer, players[currentPlayer].num[temp]);
        checkValuesForAll(players[currentPlayer].num[temp]);

        if(checkFinished()){
            return;
        }

        if(currentIndex == 4) {
            currentIndex = 0;
        }
        else
            currentIndex++;

        currentPlayer = order[currentIndex];
    }
}

//game start
void start() {

    //get random numbers for pseudo players
    int i, j;
    int temp;

    for(j=2;j<5;j++) {
        for(i=0;i<25;i++) {
            temp = rand()%99 + 1;
            while(players[j].check[temp] != 0)
                temp = rand()%99 + 1;
            players[j].num[i] = temp;
            players[j].check[players[j].num[i]] = i+1;
        }
    }

    //get play order randomly
    for(i=0;i<5;i++) {
        temp = rand()%5;
        while(checkOrder[temp]!=0) {
            temp = rand()%5;
        }

        checkOrder[temp] = 1;
        order[i] = temp;
    }
    if(order[currentIndex] != 0 && order[currentIndex] != 1)
        play(order[currentIndex], -1);
}

//show bingo status for server
void showBingo(int index) {

    int *num = players[index].num;
    int *cG = players[index].checkGot;
    int i;

    // to suppress warning
    num = num;
    cG = cG;

    printf("---------------\n");
    for(i = 0;i < 25;i++) {
        printf("%2d %c\t | ", num[i], cG[i] ? 'X' : ' ');
        if((i+1)%5 == 0)
            printf("\n");
    }
    printf("---------------\n");
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    srand(time(NULL));

    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);

    pthread_rwlock_init(&lock, NULL);

    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("\nAccepted connection from (%s, %s)\n", hostname, port);

        Pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}

//threaded program
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    game(connfd);
    Close(connfd);
    return NULL;  
}

//Robust IO using csapp.c
void RIO_writen(int fd, char response[], int stringlen) {
    if(Rio_writen(fd, response, strlen(response)) < 0)
        sendError(fd, "Rio_writen", "Server failed on Rio_writen");
}

//give proper responses to the requests
void game(int fd){
    char buf[MAXLINE], hd1[MAXLINE], hd2[MAXLINE], hd3[MAXLINE];
    //char hostname[MAXLINE], port[MAXLINE], filename[MAXLINE];
    rio_t rio;

    char response[MAXLINE];
    response[0] = '\0';

    int i, temp;
    int val;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if(Rio_readlineb(&rio, buf, MAXLINE) <= 0)
        return;
    printf("buf: %s", buf);
    sscanf(buf, "%s %s %s", hd1, hd2, hd3);

    if(!strcasecmp(hd1, "New") && !strcasecmp(hd2, "Player")) { 
        if(count == 5) {
            printf("Already Full\n");
            strcpy(response, "Already Full");
            RIO_writen(fd, response, strlen(response));
            return;
        }
        count++;
        printf("count : %d\n", count);

        if(count == 4) {
            for(i=0;i<8;i++) {
                players[0].code[i] = 40 + rand()%80;
            }
            strcpy(response, "Welcome ");
            strcat(response, players[0].code);
            strcat(response, " MainCulprit");
        }
        else {
            for(i=0;i<8;i++) {
                players[1].code[i] = 40 + rand()%80;
            }
            strcpy(response, "Welcome ");
            strcat(response, players[1].code);
            strcat(response, " CoPartner");
        }
        RIO_writen(fd, response, strlen(response));
    }
    else if(count == 5 && clientsGotVal) {
        if(finished)
        {
            strcpy(response, "Finished ");
            for(i=0;i<5;i++) {
                if(winner[i])
                    sprintf(response, "%s%d ", response, i);
            }
            strcat(response, "Prev ");
            for(i=0;i<5;i++) {
                sprintf(response, "%s%d ", response, lastFive[i]);
            }
            RIO_writen(fd, response, strlen(response));
            return;
        }
        else
        {
            printf("currentIndex %d currentPlayer %d\n", currentIndex, order[currentIndex]);
            for(i=0;i<5;i++)
                printf("%d ", order[i]);
            printf("\n");

            if(!strcmp(hd1, players[order[currentIndex]].code)) {
                if(!strcmp(hd2, "Try")) {
                    strcpy(response, "Play ");
                    if(!strcmp(hd1, players[1].code)) {
                        if(secretVal) {
                            sprintf(response, "%sMessage %d ", response, secretVal);
                            secretVal = 0;
                        }
                    }
                    for(i=0;i<5;i++) {
                        if(lastFive[i])
                            sprintf(response, "%s%d ", response, lastFive[i]);
                    }
                    RIO_writen(fd, response, strlen(response));
                }
                else if(!strcmp(hd2, "Message")) {
                    if(!strcmp(hd1, players[0].code)) {
                        secretVal = atoi(hd3);
                        strcpy(response, "Message Got");
                        RIO_writen(fd, response, strlen(response));
                    }
                }
                else if(!strcmp(hd2, "Play")) {

                    val = atoi(hd3);
                    temp = order[currentIndex];
                    play(order[currentIndex], val);
                    strcpy(response, "Got Request");
                    RIO_writen(fd, response, strlen(response));
                    for(i=0;i<5;i++)
                        showBingo(i);
                }
            }
            else if( (!strcmp(hd1, players[0].code) || !strcmp(hd1, players[1].code)) && !strcmp(hd2, "Try")) {
                strcpy(response, "Wait");
                RIO_writen(fd, response, strlen(response));
            }
            else {
                strcpy(response, "Invalid");
                printf("Invalid1\n");
                RIO_writen(fd, response, strlen(response));
            }
        }
    }
    else if(players[0].code[0] != 0 && !strcmp(hd1, players[0].code)) {
        if(!strcmp(hd2, "New")) {
            strcpy(response, "Valid ");

            //get random numbers for A
            for(i=0;i<25;i++) {
                temp = rand()%99 + 1;
                while(players[0].check[temp] != 0)
                    temp = rand()%99 + 1;
                players[0].num[i] = temp;
                players[0].check[players[0].num[i]] = 1;
                sprintf(response, "%s%d ", response, players[0].num[i]);
            }

            printf("@@here\n");
            printf("hd1: %s, codeA: %s\n", hd1, players[0].code);
            fflush(stdout);
            
            RIO_writen(fd, response, strlen(response));
            clientAGotVal = 1;
            if(clientBGotVal) {
                clientsGotVal = 1;
                start();
            }
        }
        else if(!strcmp(hd2, "Try")) {
            strcpy(response, "Wait");
            RIO_writen(fd, response, strlen(response));
        }
    }
    else if(players[1].code[0] != 0 && !strcmp(hd1, players[1].code) && !strcmp(hd2, "New")) {
        
        if(!strcmp(hd2, "New")) {
            strcpy(response, "Valid ");

            //get random numbers for B
            for(i=0;i<25;i++) {
                temp = rand()%99 + 1;
                while(players[1].check[temp] != 0)
                    temp = rand()%99 + 1;
                players[1].num[i] = temp;
                players[1].check[players[1].num[i]] = 1;
                sprintf(response, "%s%d ", response, players[1].num[i]);
            }

            printf("!!here\n");
            printf("hd1: %s, codeB: %s\n", hd1, players[1].code);
            fflush(stdout);
            RIO_writen(fd, response, strlen(response));

            clientBGotVal = 1;
            if(clientAGotVal) {
                clientsGotVal = 1;
                start();
            }
        }
        else if(!strcmp(hd2, "Try")) {
            strcpy(response, "Wait");
            RIO_writen(fd, response, strlen(response));
        }

    }
    
    else {
        strcpy(response, "Invalid");
        printf("Invalid2\n");
        RIO_writen(fd, response, strlen(response));
    }
}

//send error message to the client
void sendError(int fd, char *cause, char *longmsg) 
{
    char msg[MAXLINE];

    sprintf(msg, "Server Error");
    sprintf(msg, "%s\n%s: %s\r\n", msg, longmsg, cause);

    if(Rio_writen(fd, msg, strlen(msg)) < 0)
        return;
}
