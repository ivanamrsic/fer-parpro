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
#include <stdbool.h>

typedef enum { UNDEFINED = 0, LEFT = 1, RIGHT = 2 } Side;

typedef enum { CREATE, THINK, EAT, REQEST_RECEIVED, REQUEST_SENT, FORK_RECEIVED, FORK_SENT } PrintAction;

typedef struct {
    int rank;
    int processorCount;
    int leftNeighbor;
    int rightNeighbor;
} MyInfo;

typedef struct {
    int lastOwner;
    int whoNeedsIt;
    int isClean;
} Fork;

typedef struct {
    int processorRank;
    Side side;
} NeighborRequest;

static const NeighborRequest nullRequest = { -1, UNDEFINED };

MPI_Datatype ForkType;

MyInfo myInfo;

Fork leftFork;
Fork rightFork;

NeighborRequest incomingRequests[2];

Side sentRequestForFork;

void createForkDataType();
void initializeCutlery();
int findOutNeighbor(Side side);

void startPhilosopherLife();
void think();
void eat();

bool hasBothForks();
bool hasRequest(int index);
bool canRespondToRequest(Side side);

void getFork(Side side);
void sendFork(Side side);

void listenForMessages();
void handleReceivedMessage(Fork fork, int tag);
void addRequest(int neighbor, Side side);

void respondToRequests();
void respondTo(NeighborRequest request);

void customPrint(PrintAction action, int count, Side side);

// MARK: - Main -

int main(int argc, char *argv[]) {

    // Processor doesn't have any incoming requests.
    for (int i = 0; i < 2; i++) {
        incomingRequests[i] = nullRequest;
    }

    srand(time(0));

    MPI_Init(NULL, NULL);

    createForkDataType();

    MPI_Comm_size(MPI_COMM_WORLD, &myInfo.processorCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &myInfo.rank);

    customPrint(CREATE, -1, UNDEFINED);

    myInfo.leftNeighbor = findOutNeighbor(LEFT);
    myInfo.rightNeighbor = findOutNeighbor(RIGHT);

    initializeCutlery();

    startPhilosopherLife();

    MPI_Finalize();
}

// MARK: - Philosopher life -

void startPhilosopherLife() {
    while(1) {
        think();
        while (!hasBothForks()) {
            sleep(1);
            Side side = leftFork.whoNeedsIt == myInfo.rank ? LEFT : RIGHT;
            if (sentRequestForFork == UNDEFINED) { getFork(side); }
            listenForMessages();
        }
        eat();
        respondToRequests();
    }
}

void think() {
    int secondsToThink = rand() % 5 + 1;
    customPrint(THINK, secondsToThink, UNDEFINED);
    while (secondsToThink > 0) {
        sleep(1);
        int flag = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        if (flag != 0) { listenForMessages(); }
        secondsToThink -= 1;
    }
}

void eat() {
    // philosopher will eat between 1 and 5 seconds
    int secondsCount = rand() % 5 + 1;

    customPrint(EAT, secondsCount, UNDEFINED);
    sleep(secondsCount);

    leftFork.isClean = 0;
    rightFork.isClean = 0;
}

// MARK: - Get & send forks -

void getFork(Side side) {

    if (side == LEFT) {
        customPrint(REQUEST_SENT, -1, LEFT);
        leftFork.whoNeedsIt = myInfo.rank;
        MPI_Send(&leftFork, 1, ForkType, myInfo.leftNeighbor, 1, MPI_COMM_WORLD);
        sentRequestForFork = LEFT;
        return;
    }

    if (side == RIGHT) {
        customPrint(REQUEST_SENT, -1, RIGHT);
        rightFork.whoNeedsIt = myInfo.rank;
        MPI_Send(&rightFork, 1, ForkType, myInfo.rightNeighbor, 2, MPI_COMM_WORLD);
        sentRequestForFork = RIGHT;
    }
}

void sendFork(Side side) {

    int messageTag = side == LEFT ? 1 : 2;

    if (side == LEFT) {
        leftFork.isClean = 1;
        leftFork.lastOwner = myInfo.rank;
        leftFork.whoNeedsIt = myInfo.leftNeighbor;
        MPI_Send(&leftFork, 1, ForkType, myInfo.leftNeighbor, messageTag, MPI_COMM_WORLD);
    } else if (side == RIGHT) {
        rightFork.isClean = 1;
        rightFork.lastOwner = myInfo.rank;
        rightFork.whoNeedsIt = myInfo.rightNeighbor;
        MPI_Send(&rightFork, 1, ForkType, myInfo.rightNeighbor, messageTag, MPI_COMM_WORLD);
    }

    customPrint(FORK_SENT, -1, side);

    if (side == LEFT) {
        leftFork.whoNeedsIt = myInfo.rank;
    } else {
        rightFork.whoNeedsIt = myInfo.rank;
    }
}

// MARK: - Receive messages

void listenForMessages() {
    Fork fork;
    MPI_Status status;
    MPI_Recv(&fork, 1, ForkType, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    handleReceivedMessage(fork, status.MPI_TAG);
}

void handleReceivedMessage(Fork fork, int tag) {

    if (tag != 1 && tag != 2) { return; } // UNKNOWN TAG

    Side side = tag == 1 ? LEFT : RIGHT;

    // Message is response
    if (fork.whoNeedsIt == myInfo.rank) {

        if (side == LEFT) {
            leftFork.whoNeedsIt = -1;
            leftFork.lastOwner = myInfo.rank;
            leftFork.isClean = 1;
        } else if (side == RIGHT) {
            rightFork.whoNeedsIt = -1;
            rightFork.lastOwner = myInfo.rank;
            rightFork.isClean = 1;
        }

        customPrint(FORK_RECEIVED, -1, side);
        sentRequestForFork = UNDEFINED;
        return;
    }

    // Message is request
    customPrint(REQEST_RECEIVED, -1, side);
    addRequest(fork.whoNeedsIt, side);

    if (!canRespondToRequest(side)) { return; }

    sleep(1);
    respondToRequests();
}

void addRequest(int neighbor, Side side) {

    NeighborRequest request;
    request.processorRank = neighbor;
    request.side = side;

    int index = !hasRequest(0) ? 0 : 1;
    incomingRequests[index] = request;
}

// MARK: - Respond to request

void respondToRequests() {

    if (!hasRequest(0)) { return; }
    if (hasRequest(0)) { respondTo(incomingRequests[0]); }
    if (hasRequest(1)) { respondTo(incomingRequests[1]); }

    if (!hasRequest(0) && hasRequest(1)) {
        incomingRequests[0] = incomingRequests[1];
        incomingRequests[1] = nullRequest;
    }
}

void respondTo(NeighborRequest request) {

    if (request.side == LEFT && leftFork.isClean != 0) { return; }
    if (request.side == RIGHT && rightFork.isClean != 0) { return; }

    sendFork(request.side);
}

// MARK: - Helper functions -

bool hasBothForks() {
    return !(leftFork.whoNeedsIt == myInfo.rank) && !(rightFork.whoNeedsIt == myInfo.rank);
}

bool hasRequest(int index) {
    return incomingRequests[index].processorRank != nullRequest.processorRank && incomingRequests[index].side != UNDEFINED;
}

bool canRespondToRequest(Side side) {
    return (side == LEFT && leftFork.isClean != 1) || (side == RIGHT && rightFork.isClean != 1);
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

void initializeCutlery() {

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

int findOutNeighbor(Side side) {

    if (side == LEFT) {
        if (myInfo.rank == myInfo.processorCount - 1) { return 0; }
        return myInfo.rank + 1;
    }

    if (myInfo.rank > 0) { return myInfo.rank - 1; }
    return myInfo.processorCount - 1;
}

// MARK: - Print -

void customPrint(PrintAction action, int count, Side side) {

    for (int i = 0; i < myInfo.rank; i++) {
        printf("\t\t\t\t\t");
    }

    if (action == CREATE) {
        printf("Kreiran procesor %d, ukupno: %d\n", myInfo.rank, myInfo.processorCount);
    } else if (action == THINK) {
        printf("%d. razmislja %d sekundi\n", myInfo.rank, count);
    } else if (action == EAT) {
        printf("%d. jede %d sekundi\n", myInfo.rank, count);
    } else if (action == REQEST_RECEIVED) {
        if (side == LEFT) {
            printf("Dobiven zahtjev od %d. za lijevom.\n", myInfo.leftNeighbor);
        } else {
            printf("Dobiven zahtjev od %d. za desnom.\n", myInfo.rightNeighbor);
        }
    } else if (action == REQUEST_SENT) {
        if (side == LEFT) {
            printf("%d. trazi lijevu vilicu od %d\n", myInfo.rank, myInfo.leftNeighbor);
        } else {
            printf("%d. trazi desnu vilicu od %d\n", myInfo.rank, myInfo.rightNeighbor);
        }
    } else if (action == FORK_RECEIVED) {
        if (side == LEFT) {
            printf("%d. je primio lijevu vilicu od %d\n", myInfo.rank, myInfo.leftNeighbor);
        } else {
            printf("%d. je primio desnu vilicu od %d\n", myInfo.rank, myInfo.rightNeighbor);
        }
    } else if (action == FORK_SENT) {
        if (side == LEFT) {
            printf("%d je poslao lijevu vilicu %d\n", myInfo.rank, myInfo.leftNeighbor);
        } else {
            printf("%d je poslao desnu vilicu %d\n", myInfo.rank, myInfo.rightNeighbor);
        }
    }
}
