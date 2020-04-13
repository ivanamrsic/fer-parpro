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
    int ownerRank;
    int isClean;
} Fork;

typedef struct {
    int processorRank;
    Side side;
} NeighborRequest;

void createForkDataType();
void initializeCutlery();
int findOutNeighbor(Side side);

void startPhilosopherLife();
void think();
void eat();

bool isValid(int messageTag);
Side sideFrom(int messageTag);
bool hasBothForks();
bool hasRequest(int index);
bool canSendFork(Side side);
int neighborFor(Side side);

void requestForkFrom(Side side);
void sendForkTo(Side side);

void listenForMessages();
void handleReceivedMessage(Fork fork, int messageTag);
void addRequest(int neighbor, Side side);

void respondToRequests();
void respondTo(NeighborRequest request);

void customPrint(PrintAction action, int secondCount, int neighbor);

// MARK: - Global variables -

MPI_Datatype ForkType;

MyInfo myInfo;

Fork leftFork;
Fork rightFork;

NeighborRequest incomingRequests[2];

Side sentRequestForFork;

// MARK: - Constants -

static const NeighborRequest nullRequest = { -1, UNDEFINED };

// it is important to differentiate message sender (left vs right)
// because in case of numberOfProcessors == 2
// both left and right neighbor will be the same
static const int leftRequestForkTag = 1;
static const int leftSendForkTag = 2;
static const int rightRequestForkTag = 3;
static const int rightSendForkTag = 4;

static const int clean = 1;
static const int dirty = 2;

// MARK: - Main -

int main(int argc, char *argv[]) {

    // Processor doesn't have any incoming requests.
    for (int i = 0; i < 2; i++) { incomingRequests[i] = nullRequest; }

    srand(time(0));

    MPI_Init(NULL, NULL);

    createForkDataType();

    MPI_Comm_size(MPI_COMM_WORLD, &myInfo.processorCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &myInfo.rank);

    myInfo.leftNeighbor = findOutNeighbor(LEFT);
    myInfo.rightNeighbor = findOutNeighbor(RIGHT);

    initializeCutlery();

    customPrint(CREATE, -1, -1);

    startPhilosopherLife();

    MPI_Finalize();
}

// MARK: - Philosopher life -

void startPhilosopherLife() {
    while(1) {
        think();
        while (!hasBothForks()) {
            sleep(1);
            Side side = leftFork.ownerRank != myInfo.rank ? LEFT : RIGHT;
            if (sentRequestForFork == UNDEFINED) { requestForkFrom(side); }
            listenForMessages();
        }
        eat();
        respondToRequests();
    }
}

void think() {
    int secondsToThink = rand() % 5 + 1;
    customPrint(THINK, secondsToThink, -1);
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

    customPrint(EAT, secondsCount, -1);
    sleep(secondsCount);

    leftFork.isClean = dirty;
    rightFork.isClean = dirty;
}

// MARK: - Get & send forks -

void requestForkFrom(Side side) {
    Fork fork = { .ownerRank = myInfo.rank };
    int neighbor = neighborFor(side);
    int tag = side == LEFT ? leftRequestForkTag : rightRequestForkTag;
    MPI_Send(&fork, 1, ForkType, neighbor, tag, MPI_COMM_WORLD);
    sentRequestForFork = side;
    customPrint(REQUEST_SENT, -1, neighbor);
}

void sendForkTo(Side side) {

    Fork fork = { .ownerRank = myInfo.rank, .isClean = clean };

    int neighbor = neighborFor(side);

    int tag = side == LEFT ? leftSendForkTag : rightSendForkTag;

    MPI_Send(&fork, 1, ForkType, neighbor, tag, MPI_COMM_WORLD);

    customPrint(FORK_SENT, -1, neighbor);

    if (side == LEFT) {
        leftFork.ownerRank = -1;
        return;
    } else if (side == RIGHT) {
        rightFork.ownerRank = -1;
    }
}

// MARK: - Receive messages

void listenForMessages() {
    Fork fork;
    MPI_Status status;
    MPI_Recv(&fork, 1, ForkType, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    handleReceivedMessage(fork, status.MPI_TAG);
}

void handleReceivedMessage(Fork fork, int messageTag) {

    if (!isValid(messageTag)) { return; } // UNKNOWN TAG

    Side side = sideFrom(messageTag);

    // Message is response
    if (messageTag == leftSendForkTag || messageTag == rightSendForkTag) {

        if (side == LEFT) {
            leftFork.isClean = clean;
            leftFork.ownerRank = myInfo.rank;
        }
        else if (side == RIGHT) {
            rightFork.isClean = clean;
            rightFork.ownerRank = myInfo.rank;
        }

        customPrint(FORK_RECEIVED, -1, fork.ownerRank);
        sentRequestForFork = UNDEFINED;
        return;
    }

    // Message is request
    customPrint(REQEST_RECEIVED, -1, fork.ownerRank);
    addRequest(fork.ownerRank, side);
    respondToRequests();
}

void addRequest(int neighbor, Side side) {
    NeighborRequest request = { .processorRank = neighbor, .side = side };
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
    sleep(1);
    if (!canSendFork(request.side)) { return; }
    sendForkTo(request.side);
}

// MARK: - Helper functions -

bool isValid(int messageTag) {
    return messageTag == rightRequestForkTag
        || messageTag == leftRequestForkTag
        || messageTag == leftSendForkTag
        || messageTag == rightSendForkTag;
}

Side sideFrom(int messageTag) {
    if (messageTag == leftRequestForkTag) { return RIGHT; }
    if (messageTag == rightRequestForkTag) { return LEFT; }
    if (messageTag == leftSendForkTag) { return RIGHT; }
    if (messageTag == rightSendForkTag) { return LEFT; }
    return UNDEFINED;
}

bool hasLeftFork() { return leftFork.ownerRank == myInfo.rank; }

bool hasRightFork() { return rightFork.ownerRank == myInfo.rank; }

bool hasBothForks() { return hasLeftFork() && hasRightFork(); }

bool hasRequest(int index) {
    return incomingRequests[index].processorRank != nullRequest.processorRank && incomingRequests[index].side != UNDEFINED;
}

bool canSendFork(Side side) {
    if (side == LEFT) { return hasLeftFork() && leftFork.isClean == dirty; }
    else if (side == RIGHT) { return hasRightFork() && rightFork.isClean == dirty; }
    return false;
}

int neighborFor(Side side) {
    return side == LEFT ? myInfo.leftNeighbor : myInfo.rightNeighbor;
}

// MARK: - Initialization -

void createForkDataType() {

    MPI_Datatype type[2] = { MPI_INT, MPI_INT };
    int blocklen[2] = { 1, 1 };
    MPI_Aint disp[2];

    disp[0] = 0;
    disp[1] = 1;

    MPI_Type_create_struct(2, blocklen, disp, type, &ForkType);
    MPI_Type_commit(&ForkType);
}

void initializeCutlery() {

    leftFork.isClean = dirty;
    rightFork.isClean = dirty;

    leftFork.ownerRank = -1;
    rightFork.ownerRank = -1;

    if (myInfo.rank == myInfo.processorCount - 1) { return; }

    if (myInfo.rank == 0) {
        rightFork.ownerRank = myInfo.rank;
    }

    leftFork.ownerRank = myInfo.rank;
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

void customPrint(PrintAction action, int secondCount, int neighbor) {

    for (int i = 0; i < myInfo.rank; i++) { printf("\t\t\t\t"); }

    if (action == CREATE) {
        printf("Kreiran procesor %d, ukupno: %d\n", myInfo.rank, myInfo.processorCount);
    } else if (action == THINK) {
        printf("%d. razmislja %d sekundi\n", myInfo.rank, secondCount);
    } else if (action == EAT) {
        printf("%d. jede %d sekundi\n", myInfo.rank, secondCount);
    } else if (action == REQEST_RECEIVED) {
        printf("Dobiven zahtjev od %d.\n", neighbor);
    } else if (action == REQUEST_SENT) {
        printf("%d. trazi vilicu od %d.\n", myInfo.rank, neighbor);
    } else if (action == FORK_RECEIVED) {
        printf("%d. je primio vilicu od %d.\n", myInfo.rank, neighbor);
    } else if (action == FORK_SENT) {
        printf("%d je poslao vilicu %d.\n", myInfo.rank, neighbor);
    }
}
