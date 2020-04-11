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

typedef struct {
    int isClean;
    int hasIt;
    int lastOwner;
} Fork;

typedef struct {
    int rank;
    int processorCount;
    int leftNeighbor;
    int rightNeighbor;
} MyInfo;

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
void handleReceivedForkMessage(Fork fork);
void respondToRequests();
void sendRightFork();
void sendLeftFork();
void addRequest(Fork fork);

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
        while (!leftFork.hasIt || !rightFork.hasIt) {
            getForks();
        }
        eat();
        respondToRequests();
    }
}

// MARK: - Chores

void think() {
    //            i 'istovremeno' odgovaraj na zahtjeve!            // asinkrono, s povremenom provjerom (vidi nastavak)

    // philosopher will think between 1 and 10 seconds
    int secondsCount = rand() % 10 + 1;
    printf("Procesor %d razmislja %d sekundi.\n", myInfo.rank, secondsCount);
    sleep(secondsCount);
}


void getForks() {

    if (!leftFork.hasIt && leftFork.lastOwner == myInfo.rank) {
        printf("Procesor %d trazi lijevu vilicu.\n", myInfo.rank);
        MPI_Send(0, 1, MPI_INT, myInfo.leftNeighbor, 0, MPI_COMM_WORLD);
    }

    if (!rightFork.hasIt && rightFork.lastOwner == myInfo.rank) {
        printf("Procesor %d trazi desnu vilicu.\n", myInfo.rank);
        MPI_Send(0, 1, MPI_INT, myInfo.rightNeighbor, 0, MPI_COMM_WORLD);
    }

    while (1) {

        Fork fork;
        MPI_Recv(&fork, 1, ForkType, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        handleReceivedForkMessage(fork);
    }
}

void eat() {
    // philosopher will think between 1 and 10 seconds
    int secondsCount = rand() % 10 + 1;
    printf("Procesor %d jede %d sekundi.\n", myInfo.rank, secondsCount);
    sleep(secondsCount);

    leftFork.isClean = 0;
    rightFork.isClean = 0;
}

void handleReceivedForkMessage(Fork fork) {

    // Poruka je zahtjev
    if (!fork.hasIt) {
//                inace ako je poruka zahtjev            // drugi traze moju vilicu
//                    obradi zahtjev (odobri ili zabiljezi);
        addRequest(fork);
        return;
    }

    // Poruka je odgovor

    if (fork.lastOwner == myInfo.leftNeighbor) {
        leftFork.hasIt = 1;
        leftFork.lastOwner = myInfo.rank;
        return;
    }

    if (fork.lastOwner == myInfo.rightNeighbor) {
        rightFork.hasIt = 1;
        rightFork.lastOwner = myInfo.rank;
    }
}

void respondToRequests() {

    if (requests[0] == -1) { return; }

    if (requests[0] == myInfo.leftNeighbor && leftFork.hasIt == 1) {
        sendLeftFork();
    }

    if (requests[0] == myInfo.rightNeighbor && rightFork.hasIt == 1) {
        sendRightFork();
    }

    if (requests[1] == myInfo.leftNeighbor && leftFork.hasIt == 1) {
        sendLeftFork();
    }

    if (requests[1] == myInfo.rightNeighbor && rightFork.hasIt == 1) {
        sendRightFork();
    }

    if (requests[0] == -1 && requests[1] != -1) {
        requests[0] = requests[1];
        requests[1] = -1;
    }
}

void sendRightFork() {

    rightFork.isClean = 1;
    rightFork.lastOwner = myInfo.rank;

    MPI_Send(&rightFork, 1, ForkType, myInfo.rightNeighbor, 0, MPI_COMM_WORLD);

    rightFork.hasIt = 0;
    requests[1] = -1;
}

void sendLeftFork() {

    leftFork.isClean = 1;
    leftFork.lastOwner = myInfo.rank;

    MPI_Send(&leftFork, 1, ForkType, myInfo.leftNeighbor, 0, MPI_COMM_WORLD);

    rightFork.hasIt = 0;
    requests[1] = -1;
}

void addRequest(Fork fork) {
    int index = requests[0] == -1 ? 0 : 1;
    requests[index] = fork.lastOwner;
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

    if (myInfo.rank == myInfo.processorCount - 1) { return; }

    if (myInfo.rank == 0) {
        rightFork.isClean = 1;
        rightFork.hasIt = 1;
        rightFork.lastOwner = myInfo.rank;
    }

    leftFork.isClean = 1;
    leftFork.hasIt = 1;
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
