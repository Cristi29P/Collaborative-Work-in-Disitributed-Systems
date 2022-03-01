#include "mpi.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <iterator>
#include <string>
#include <math.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#define COORD0 0
#define COORD1 1
#define COORD2 2

// Define certain actions
#define LEARN_COORDINATOR 10

#define MSG_FOR_1_FROM_0 101
#define MSG_FOR_2_FROM_0 102
#define MSG_FOR_0_FROM_1 110
#define MSG_FOR_2_FROM_1 112
#define MSG_FOR_0_FROM_2 120
#define MSG_FOR_1_FROM_2 121

#define RECV_TOPO_0 30

#define DATA_FLOW 40

typedef struct Topology {
    std::vector<int> clusters[3];
} Topology;

void read_input(std::vector<int>& cluster_workers, int rank);

void log_message(MPI_Status status, int rank);

void print_topology(Topology topologie, int rank);

void calculate_balance(std::vector<int>& factors, Topology topologie, int vector_size);

void print_result(std::vector<int>& result);

void send_to_workers(std::vector<int>& cluster_workers, std::vector<int>& recv_vector_cluster);

void recv_from_workers(std::vector<int>& cluster_workers, std::vector<int>& cluster_from_workers, int rank);

void recv_info_vector(std::vector<int>& recv_vector, int rank, int source, int tag, int* recv_size);

void send_info_vector(std::vector<int>& send_vector, int *size, int dest, int tag, int offset);

void init_vector(std::vector<int>& source_vector, int vector_size);

void reconstruct_vector(std::vector<int>& dest_vector, int rank, int source, int tag, int* recv_size);