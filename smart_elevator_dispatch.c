#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_FLOORS 11       // Floors 0 to 10 (0 = Ground Floor)
#define NUM_ELEVATORS 3    // 3 Elevators: E1 (Local), E2 (Express), E3 (Local)
#define MAX_QUEUE 20

// DATA STRUCTURES

typedef struct {
    int id;                 // 1, 2, 3
    int currentFloor;       // Current location (0 - 10)
    int capacity;           // Maximum passenger capacity
    int currentLoad;        // Current passengers inside
    bool isExpress;         // True = Express lift, False = Local lift
    bool available;         // True = Idle/Free, False = Busy
} Elevator;

typedef struct {
    int requestId;
    int pickupFloor;
    int destFloor;
    int priority;           // 1 = Emergency, 0 = Normal
} Request;

// Normal Request FIFO Queue
typedef struct {
    Request items[MAX_QUEUE];
    int front;
    int rear;
    int count;
} Queue;

// Emergency Request Priority Queue
typedef struct {
    Request items[MAX_QUEUE];
    int size;
} PriorityQueue;

void initQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

bool isQueueEmpty(Queue* q) {
    return (q->count == 0);
}

bool isQueueFull(Queue* q) {
    return (q->count == MAX_QUEUE);
}

void enqueueNormal(Queue* q, Request req) {
    if (isQueueFull(q)) {
        printf("[NOTICE] Normal request queue is full. Please try again later.\n");
        return;
    }
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->items[q->rear] = req;
    q->count++;
}

bool dequeueNormal(Queue* q, Request* outReq) {
    if (isQueueEmpty(q)) return false;
    *outReq = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->count--;
    return true;
}

void initPriorityQueue(PriorityQueue* pq) {
    pq->size = 0;
}

bool isPriorityQueueEmpty(PriorityQueue* pq) {
    return (pq->size == 0);
}

void insertEmergency(PriorityQueue* pq, Request req) {
    if (pq->size >= MAX_QUEUE) {
        printf("[NOTICE] Emergency queue is full.\n");
        return;
    }
    pq->items[pq->size] = req;
    pq->size++;
}

bool extractEmergency(PriorityQueue* pq, Request* outReq) {
    if (pq->size <= 0) return false;
    *outReq = pq->items[0];
    for (int i = 0; i < pq->size - 1; i++) {
        pq->items[i] = pq->items[i + 1];
    }
    pq->size--;
    return true;
}
int routeGraph[NUM_ELEVATORS][NUM_FLOORS][NUM_FLOORS];

void initBuildingGraph(void) {
    for (int e = 0; e < NUM_ELEVATORS; e++) {
        for (int i = 0; i < NUM_FLOORS; i++) {
            for (int j = 0; j < NUM_FLOORS; j++) {
                routeGraph[e][i][j] = (i == j) ? 1 : 0;
            }
        }
    }

    // E1 and E3 (Local): Stop at every adjacent floor
    for (int e = 0; e < NUM_ELEVATORS; e += 2) {
        for (int f = 0; f < NUM_FLOORS - 1; f++) {
            routeGraph[e][f][f + 1] = 1;
            routeGraph[e][f + 1][f] = 1;
        }
    }

    // E2 (Express): Stops at Ground (0), Mid (5), and Roof (10)
    routeGraph[1][0][5] = 1;
    routeGraph[1][5][0] = 1;
    routeGraph[1][5][10] = 1;
    routeGraph[1][10][5] = 1;
}

bool findShortestPath(int elevatorIdx, int startFloor, int targetFloor, int path[], int* pathLen) {
    if (startFloor == targetFloor) {
        path[0] = startFloor;
        *pathLen = 1;
        return true;
    }

    bool visited[NUM_FLOORS] = {false};
    int parent[NUM_FLOORS];
    int queue[NUM_FLOORS];
    int head = 0, tail = 0;

    for (int i = 0; i < NUM_FLOORS; i++) parent[i] = -1;

    visited[startFloor] = true;
    queue[tail++] = startFloor;

    bool reached = false;
    while (head < tail) {
        int current = queue[head++];
        if (current == targetFloor) {
            reached = true;
            break;
        }

        for (int next = 0; next < NUM_FLOORS; next++) {
            if (routeGraph[elevatorIdx][current][next] && !visited[next]) {
                visited[next] = true;
                parent[next] = current;
                queue[tail++] = next;
            }
        }
    }

    if (!reached) return false;

    int temp[NUM_FLOORS];
    int count = 0;
    int curr = targetFloor;
    while (curr != -1) {
        temp[count++] = curr;
        curr = parent[curr];
    }

    for (int i = 0; i < count; i++) {
        path[i] = temp[count - 1 - i];
    }
    *pathLen = count;
    return true;
}

typedef struct {
    int elevatorIndex;
    int distance;
} Candidate;

void sortCandidates(Candidate candidates[], int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (candidates[j].distance > candidates[j + 1].distance) {
                Candidate temp = candidates[j];
                candidates[j] = candidates[j + 1];
                candidates[j + 1] = temp;
            }
        }
    }
}

int findBestElevator(Elevator elevators[], int pickupFloor, int destFloor) {
    Candidate eligible[NUM_ELEVATORS];
    int eligibleCount = 0;

    for (int i = 0; i < NUM_ELEVATORS; i++) {
        if (!elevators[i].available) continue;

        int testPath[NUM_FLOORS], len;
        bool canReachPickup = findShortestPath(i, elevators[i].currentFloor, pickupFloor, testPath, &len);
        bool canReachDest = findShortestPath(i, pickupFloor, destFloor, testPath, &len);

        if (canReachPickup && canReachDest) {
            eligible[eligibleCount].elevatorIndex = i;
            eligible[eligibleCount].distance = abs(elevators[i].currentFloor - pickupFloor);
            eligibleCount++;
        }
    }

    if (eligibleCount == 0) return -1;

    sortCandidates(eligible, eligibleCount);
    return eligible[0].elevatorIndex;
}

void printPath(const int path[], int length) {
    for (int i = 0; i < length; i++) {
        printf("Floor %d%s", path[i], (i == length - 1) ? "" : " -> ");
    }
    printf("\n");
}

bool serveRequest(Elevator elevators[], Request req) {
    if (req.priority == 1) {
        printf(">>> DISPATCHING EMERGENCY REQUEST #%d <<<\n", req.requestId);
        printf("Priority Alert: Immediate response required for Floor %d!\n", req.pickupFloor);
    } else {
        printf(">>> DISPATCHING NORMAL REQUEST #%d <<<\n", req.requestId);
    }
    printf("Passenger Request: Floor %d to Floor %d\n", req.pickupFloor, req.destFloor);

    int bestIdx = findBestElevator(elevators, req.pickupFloor, req.destFloor);

    if (bestIdx == -1) {
        printf("[NOTICE] No elevator can currently serve this trip.\n");
        printf("         Reason: Either all elevators are busy, or the requested\n");
        printf("         floors are not accessible by the available lifts.\n");
        return false;
    }

    Elevator* e = &elevators[bestIdx];
    int toPickupPath[NUM_FLOORS], pickupLen = 0;
    int toDestPath[NUM_FLOORS], destLen = 0;

    findShortestPath(bestIdx, e->currentFloor, req.pickupFloor, toPickupPath, &pickupLen);
    findShortestPath(bestIdx, req.pickupFloor, req.destFloor, toDestPath, &destLen);

    int distanceToPickup = abs(e->currentFloor - req.pickupFloor);

    printf("Selected Elevator : E%d (%s Lift)\n", e->id, e->isExpress ? "Express" : "Local");
    printf("Starting Location : Floor %d\n", e->currentFloor);
    printf("Selection Reason  : Closest available lift (%d floor%s away)\n",
           distanceToPickup, (distanceToPickup == 1) ? "" : "s");

    printf("\n[Step 1] Elevator moving to pick up passenger:\n  Route: ");
    printPath(toPickupPath, pickupLen);
    printf("  Status: Arrived at Floor %d. Passenger boarded.\n", req.pickupFloor);

    printf("\n[Step 2] Elevator traveling to destination:\n  Route: ");
    printPath(toDestPath, destLen);
    printf("  Status: Arrived at Floor %d. Passenger exited.\n", req.destFloor);

    e->currentFloor = req.destFloor;
    e->available = true;

    printf("\n[Result] Trip complete! Elevator E%d is now free at Floor %d.\n", e->id, e->currentFloor);

    return true;
}

void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void displayElevators(const Elevator elevators[], int n) {
    printf("                      ELEVATOR FLEET STATUS                      \n");
    printf("%-6s | %-10s | %-15s | %-12s | %-10s\n",
           "Lift", "Type", "Current Floor", "Capacity", "Status");

    for (int i = 0; i < n; i++) {
        printf("E%-5d | %-10s | Floor %-9d | %d persons   | %-10s\n",
               elevators[i].id,
               elevators[i].isExpress ? "Express" : "Local",
               elevators[i].currentFloor,
               elevators[i].capacity,
               elevators[i].available ? "Available" : "Busy");
    }
}

void displayBuildingInfo(void) {
    printf("                     BUILDING & ROUTE GUIDE                      \n");
    printf("Floors in Building : Floor 0 (Ground) to Floor 10 (Roof)\n\n");
    printf("Elevator Types & Stops:\n");
    printf("  - E1 (Local)   : Stops at every floor (0, 1, 2, 3, ..., 10)\n");
    printf("  - E2 (Express) : Express hub service only (Stops at 0, 5, 10)\n");
    printf("  - E3 (Local)   : Stops at every floor (0, 1, 2, 3, ..., 10)\n\n");
    printf("Priority Queue System:\n");
    printf("  - Emergency requests (Priority 1) jump ahead of normal requests.\n");
    printf("  - Normal requests (Priority 0) are processed in FIFO order.\n");
}


int main(void) {
    Elevator elevators[NUM_ELEVATORS] = {
        {1, 0,  8, 0, false, true}, // E1: Local, at Floor 0
        {2, 5, 12, 0, true,  true}, // E: Express, at Floor 5
        {3, 10, 8, 0, false, true}  // E3: Local, at Floor 10
    };

    Queue normalQueue;
    PriorityQueue emergencyQueue;

    initQueue(&normalQueue);
    initPriorityQueue(&emergencyQueue);
    initBuildingGraph();

    int requestCounter = 1;
    int choice;

    printf("         SMART ELEVATOR DISPATCH SYSTEM             \n");
        printf("\nMAIN MENU:\n");
        printf("1. Request Elevator (Instant Ride)\n");
        printf("2. Queue a Request (Normal or Emergency)\n");
        printf("3. Process Next Queued Request\n");
        printf("4. Display Elevator Fleet Status\n");
        printf("5. View Building & Route Guide\n");
        printf("6. Exit\n");

    while (1) {
        printf("\nEnter your choice (1-6): ");

        if (scanf("%d", &choice) != 1) {
            printf("\n[ERROR] Invalid input! Please enter a number between 1 and 6.\n");
            clearInputBuffer();
            continue;
        }

        switch (choice) {
            case 1: { // Instant Request
                int pickup, dest, priority;

                printf("\nEnter your current floor (0 to 10): ");
                if (scanf("%d", &pickup) != 1 || pickup < 0 || pickup >= NUM_FLOORS) {
                    printf("[ERROR] Invalid floor! Must be between 0 and 10.\n");
                    clearInputBuffer();
                    break;
                }

                printf("Enter your destination floor (0 to 10): ");
                if (scanf("%d", &dest) != 1 || dest < 0 || dest >= NUM_FLOORS) {
                    printf("[ERROR] Invalid floor! Must be between 0 and 10.\n");
                    clearInputBuffer();
                    break;
                }

                if (pickup == dest) {
                    printf("[ERROR] You are already on Floor %d! Pickup and destination must differ.\n", pickup);
                    break;
                }

                printf("Is this an emergency? (1 = Emergency, 0 = Normal): ");
                if (scanf("%d", &priority) != 1 || (priority != 0 && priority != 1)) {
                    printf("[ERROR] Invalid priority! Enter 1 for Emergency or 0 for Normal.\n");
                    clearInputBuffer();
                    break;
                }

                Request req = {requestCounter++, pickup, dest, priority};
                serveRequest(elevators, req);
                break;
            }

            case 2: { // Queue a request
                int pickup, dest, priority;

                printf("\nEnter pickup floor (0 to 10): ");
                if (scanf("%d", &pickup) != 1 || pickup < 0 || pickup >= NUM_FLOORS) {
                    printf("[ERROR] Invalid floor! Must be between 0 and 10.\n");
                    clearInputBuffer();
                    break;
                }

                printf("Enter destination floor (0 to 10): ");
                if (scanf("%d", &dest) != 1 || dest < 0 || dest >= NUM_FLOORS) {
                    printf("[ERROR] Invalid floor! Must be between 0 and 10.\n");
                    clearInputBuffer();
                    break;
                }

                if (pickup == dest) {
                    printf("[ERROR] Pickup and destination floors cannot be identical.\n");
                    break;
                }

                printf("Select request priority (1 = Emergency, 0 = Normal): ");
                if (scanf("%d", &priority) != 1 || (priority != 0 && priority != 1)) {
                    printf("[ERROR] Invalid priority! Enter 1 or 0.\n");
                    clearInputBuffer();
                    break;
                }

                Request req = {requestCounter++, pickup, dest, priority};
                if (priority == 1) {
                    insertEmergency(&emergencyQueue, req);
                    printf("\n[SUCCESS] EMERGENCY Request #%d (Floor %d -> %d) added to Priority Queue!\n",
                           req.requestId, req.pickupFloor, req.destFloor);
                } else {
                    enqueueNormal(&normalQueue, req);
                    printf("\n[SUCCESS] Normal Request #%d (Floor %d -> %d) added to Standard FIFO Queue.\n",
                           req.requestId, req.pickupFloor, req.destFloor);
                }
                printf("Pending Requests: %d Emergency | %d Normal\n",
                       emergencyQueue.size, normalQueue.count);
                break;
            }

            case 3: { // Process queued request
                if (isPriorityQueueEmpty(&emergencyQueue) && isQueueEmpty(&normalQueue)) {
                    printf("\n[INFO] Both queues are empty. No pending requests to process.\n");
                    break;
                }

                Request activeReq;
                if (extractEmergency(&emergencyQueue, &activeReq)) {
                    printf("\n[PRIORITY QUEUE] Processing HIGH-PRIORITY Emergency Request #%d ahead of normal queue!\n",
                           activeReq.requestId);
                    serveRequest(elevators, activeReq);
                } else if (dequeueNormal(&normalQueue, &activeReq)) {
                    printf("\n[NORMAL QUEUE] Processing Standard Request #%d in FIFO order...\n",
                           activeReq.requestId);
                    serveRequest(elevators, activeReq);
                }
                break;
            }

            case 4: { // Display status
                displayElevators(elevators, NUM_ELEVATORS);
                printf("Queue Status: %d Emergency waiting, %d Normal waiting\n",
                       emergencyQueue.size, normalQueue.count);
                break;
            }

            case 5: { // View info
                displayBuildingInfo();
                break;
            }

            case 6: { // Exit
                printf("\nThank you for using the Smart Elevator Dispatch System!\n");
                return 0;
            }

            default: {
                printf("\n[ERROR] Choice out of range! Enter a number between 1 and 6.\n");
                break;
            }
        }
    }

    return 0;
}