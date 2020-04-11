//
//  philosopher.c
//  
//
//  Created by Ivana Mršić on 11/04/2020.
//

#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// tags. 1 - left , 2 - right

typedef struct {
    int lastOwner;
    int whoNeedsIt;
    int isClean;
} Fork;

typedef struct {
    int rank;
    int processorCount;
    int leftNeighbor;
    int rightNeighbor;
} MyInfo;

int leftRequestSent;
int rightRequestSent;

Fork leftFork;
Fork rightFork;

int requests[2];

MPI_Datatype ForkType;

MyInfo myInfo;

void createForkDataType();
void fillRequestsArray();
void setupCutlery();
int findOutLeftNeighbor();
int findOutRightNeighbor();

void startPhilosopherLife();
void think();
void eat();
void getForks();
void handleReceivedForkMessage(Fork fork, int tag);
void respondToRequests();
void sendRightFork();
void sendLeftFork();
void addRequest(Fork fork);
void listenForRequest();

// MARK: - Main -

int main(int argc, char *argv[]) {

    fillRequestsArray();

    srand(time(0));

    MPI_Init(NULL, NULL);

    createForkDataType();

    MPI_Comm_size(MPI_COMM_WORLD, &myInfo.processorCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &myInfo.rank);

    printf("Hello world from processor %d out of %d processors.\n", myInfo.rank, myInfo.processorCount);

    myInfo.leftNeighbor = findOutLeftNeighbor();
    myInfo.rightNeighbor = findOutRightNeighbor();

    setupCutlery();

    startPhilosopherLife();

    MPI_Finalize();
}

void startPhilosopherLife() {
    while(1) {
        think();
        while (leftFork.whoNeedsIt == myInfo.rank || rightFork.whoNeedsIt == myInfo.rank) {
            sleep(1);
            getForks();
        }
        eat();
        respondToRequests();
    }
}

// MARK: - Chores

void getForks() {

    if (leftFork.whoNeedsIt == myInfo.rank && leftRequestSent != 1) {
        printf("Procesor %d trazi lijevu vilicu od %d procesora.\n", myInfo.rank, myInfo.leftNeighbor);
        MPI_Send(&leftFork, 1, ForkType, myInfo.leftNeighbor, 1, MPI_COMM_WORLD);
        leftRequestSent = 1;
    }

    if (rightFork.whoNeedsIt == myInfo.rank && rightRequestSent != 1) {
        printf("Procesor %d trazi desnu vilicu od %d procesora.\n", myInfo.rank, myInfo.rightNeighbor);
        MPI_Send(&rightFork, 1, ForkType, myInfo.rightNeighbor, 2, MPI_COMM_WORLD);
        rightRequestSent = 1;
    }

    listenForRequest();
}

void listenForRequest() {
    Fork fork;
    MPI_Status status;
    MPI_Recv(&fork, 1, ForkType, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    handleReceivedForkMessage(fork, status.MPI_TAG);
}

void handleReceivedForkMessage(Fork fork, int tag) {

    // Poruka je zahtjev
    if (fork.whoNeedsIt != myInfo.rank) {
        printf("Dobiven zahtjev od %d za procesor %d.\n", fork.whoNeedsIt, myInfo.rank);

//                inace ako je poruka zahtjev            // drugi traze moju vilicu
//                    obradi zahtjev (odobri ili zabiljezi);
        addRequest(fork);
        return;
    }

    // Poruka je odgovor
    if (fork.lastOwner == myInfo.leftNeighbor && tag == 1) {
        printf("Procesor %d je primio lijevu vilicu od %d procesora.\n", myInfo.rank, fork.lastOwner);
        leftFork.whoNeedsIt = -1;
        leftFork.lastOwner = myInfo.rank;
        leftRequestSent = 0;
        return;
    } else if (fork.lastOwner == myInfo.rightNeighbor && tag == 2) {
        printf("Procesor %d je primio desnu vilicu od %d procesora.\n", myInfo.rank, fork.lastOwner);
        rightFork.whoNeedsIt = -1;
        rightFork.lastOwner = myInfo.rank;
        rightRequestSent = 0;
    }
}


void think() {
    //            i 'istovremeno' odgovaraj na zahtjeve!            // asinkrono, s povremenom provjerom (vidi nastavak)

    // philosopher will think between 1 and 5 seconds
    int secondsCount = rand() % 5 + 1;
    printf("Procesor %d razmislja %d sekundi.\n", myInfo.rank, secondsCount);

    double time = 1000 * secondsCount;

    while (time > 0.0) {
        usleep(1000);

        int flag = 0;

        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);

        if (flag != 0) {
            listenForRequest();
        }

        time -= 1000.0;
    }

    sleep(secondsCount);
}

void eat() {
    // philosopher will think between 1 and 10 seconds
    int secondsCount = rand() % 5 + 1;
    printf("Procesor %d jede %d sekundi.\n", myInfo.rank, secondsCount);
    sleep(secondsCount);

    leftFork.isClean = 0;
    rightFork.isClean = 0;
}


void respondToRequests() {

    if (requests[0] == -1) { return; }

    if (requests[0] == myInfo.leftNeighbor && leftFork.lastOwner == myInfo.rank && leftFork.whoNeedsIt != myInfo.rank) {
        sendLeftFork();
        requests[0] = -1;
    }

    if (requests[0] == myInfo.rightNeighbor && rightFork.lastOwner == myInfo.rank && rightFork.whoNeedsIt != myInfo.rank) {
        sendRightFork();
        requests[0] = -1;
    }

    if (requests[1] == myInfo.leftNeighbor && leftFork.lastOwner == myInfo.rank && leftFork.whoNeedsIt != myInfo.rank) {
        sendLeftFork();
        requests[1] = -1;
    }

    if (requests[1] == myInfo.rightNeighbor && rightFork.lastOwner == myInfo.rank && rightFork.whoNeedsIt != myInfo.rank) {
        sendRightFork();
        requests[1] = -1;
    }

    if (requests[0] == -1 && requests[1] != -1) {
        requests[0] = requests[1];
        requests[1] = -1;
    }
}

void sendRightFork() {

    rightFork.isClean = 1;
    rightFork.lastOwner = myInfo.rank;
    rightFork.whoNeedsIt = myInfo.rightNeighbor;

    printf("Procesor %d je poslao desnu vilicu %d procesoru.\n", myInfo.rank, myInfo.rightNeighbor);
    MPI_Send(&rightFork, 1, ForkType, myInfo.rightNeighbor, 2, MPI_COMM_WORLD);

    rightFork.whoNeedsIt = myInfo.rank;
}

void sendLeftFork() {

    leftFork.isClean = 1;
    leftFork.lastOwner = myInfo.rank;
    leftFork.whoNeedsIt = myInfo.leftNeighbor;

    printf("Procesor %d je poslao lijevu vilicu %d procesoru.\n", myInfo.rank, myInfo.leftNeighbor);
    MPI_Send(&leftFork, 1, ForkType, myInfo.leftNeighbor, 1, MPI_COMM_WORLD);

    leftFork.whoNeedsIt = myInfo.rank;
}

void addRequest(Fork fork) {
    int index = requests[0] == -1 ? 0 : 1;
    requests[index] = fork.whoNeedsIt;
}

// MARK: - Initialization -

void createForkDataType() {

    MPI_Datatype type[3] = { MPI_INT, MPI_INT, MPI_INT };
    int blocklen[3] = { 1, 1, 1 };
    MPI_Aint disp[3];

    disp[0] = 0;
    disp[1] = 1;
    disp[2] = 2;

    MPI_Type_create_struct(3, blocklen, disp, type, &ForkType);
    MPI_Type_commit(&ForkType);
}

void setupCutlery() {

    rightFork.whoNeedsIt = myInfo.rank;
    leftFork.whoNeedsIt = myInfo.rank;

    leftFork.lastOwner = myInfo.leftNeighbor;
    rightFork.lastOwner = myInfo.rightNeighbor;

    if (myInfo.rank == myInfo.processorCount - 1) { return; }

    if (myInfo.rank == 0) {
        rightFork.isClean = 1;
        rightFork.whoNeedsIt = -1;
        rightFork.lastOwner = myInfo.rank;
    }

    leftFork.isClean = 1;
    leftFork.whoNeedsIt = -1;
    leftFork.lastOwner = myInfo.rank;
}

int findOutLeftNeighbor() {
    if (myInfo.rank == myInfo.processorCount - 1) { return 0; }
    return myInfo.rank + 1;
}

int findOutRightNeighbor() {
    if (myInfo.rank > 0) { return myInfo.rank - 1; }
    return myInfo.processorCount - 1;
}

void fillRequestsArray() {
    for (int i = 0; i < 2; i++) {
        requests[i] = -1;
    }
}
