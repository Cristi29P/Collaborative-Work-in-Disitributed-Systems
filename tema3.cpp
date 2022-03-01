#include "tema3_utils.h"

int main(int argc, char *argv[]) {
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int vector_size = std::stoi(argv[1]), my_coordinator = -1, recv_size = -1; 
    Topology topologie;
    std::vector<int> cluster_workers, source_vector, balance_factors, final_vector;

    // Each coordinators learns about his workers and lets them know the cluster they belong to
    if (rank <= 2) {
        read_input(cluster_workers, rank);
        topologie.clusters[rank] = cluster_workers;

        for (const auto& my_worker : cluster_workers) {
            MPI_Send(&rank, 1, MPI_INT, my_worker, LEARN_COORDINATOR, MPI_COMM_WORLD);
        }
    } else {
        // Each workers learns the cluster and the coordinator hey belong to
        MPI_Status status;
        MPI_Recv(&my_coordinator, 1, MPI_INT, MPI_ANY_SOURCE, LEARN_COORDINATOR, MPI_COMM_WORLD, &status);
        log_message(status, rank); // the messages are logged when they are received
    }
    
    // The coordinators share info about their clusters
    int size = cluster_workers.size();
    for (int i = 0; i <= 2; i++) {
        send_info_vector(cluster_workers, &size, (rank == COORD2) ? i : COORD2, 100 + 10 * rank + i, 0);
    }

    if (rank == COORD0) {
        // Receive info from cluster 1
        recv_info_vector(topologie.clusters[1], rank, COORD2, MSG_FOR_0_FROM_1, &recv_size);
        // Receive info frum cluster 2
        recv_info_vector(topologie.clusters[2], rank, COORD2, MSG_FOR_0_FROM_2, &recv_size);
    }

    if (rank == COORD1) {
        // Receive info from cluster 0
        recv_info_vector(topologie.clusters[0], rank, COORD2, MSG_FOR_1_FROM_0, &recv_size);
        // Receive info frum cluster 2
        recv_info_vector(topologie.clusters[2], rank, COORD2, MSG_FOR_1_FROM_2, &recv_size);
    }

    if (rank == COORD2) {
        // Receive info from cluster 0
        recv_info_vector(topologie.clusters[0], rank, COORD0, MSG_FOR_2_FROM_0, &recv_size);
        // Receive info from cluster 1
        recv_info_vector(topologie.clusters[1], rank, COORD1, MSG_FOR_2_FROM_1, &recv_size);
        // Rerouting info from 0 to 1 and from 1 to 0
        std::vector<int> rerouting_cluster;
        recv_info_vector(rerouting_cluster, rank, COORD0, MSG_FOR_1_FROM_0, &recv_size);
        send_info_vector(rerouting_cluster, &recv_size, COORD1, MSG_FOR_1_FROM_0, 0);
        recv_info_vector(rerouting_cluster, rank, COORD1, MSG_FOR_0_FROM_1, &recv_size);
        send_info_vector(rerouting_cluster, &recv_size, COORD0, MSG_FOR_0_FROM_1, 0);
    }

    // The coordinators know the full topology
    if (rank <= 2) {
        print_topology(topologie, rank);
    }

    // Each cluster coordinator then sends the received info to his workers
    if (rank <= 2) {
        for (int j = 0; j <= 2; j++) {
            for (const auto& worker : cluster_workers) {
                int topo_size = topologie.clusters[j].size();
                send_info_vector(topologie.clusters[j], &topo_size, worker, RECV_TOPO_0 + j, 0);
            }
        }
    } else { // The workers receive the topology info from their coordinators
        for (int j = 0; j <= 2; j++) {
            recv_info_vector(topologie.clusters[j], rank, my_coordinator, RECV_TOPO_0 + j, &recv_size);
        }
    }
    // Each workers prints the full topology
    if (rank > 2) {
        print_topology(topologie, rank);
    }

    if (rank == COORD0) {
        init_vector(source_vector, vector_size); // Creates the vector that will be doubled
        
        // Calculate the balance factors
        calculate_balance(balance_factors, topologie, vector_size);
        int offset2 = balance_factors.at(0) + balance_factors.at(1);

        // Send directly to COORD2
        send_info_vector(source_vector, &balance_factors[2], COORD2, MSG_FOR_2_FROM_0, offset2);
        // Send indirectly to COORD1 through COORD2
        send_info_vector(source_vector, &balance_factors[1], COORD2, MSG_FOR_1_FROM_0, balance_factors.at(0));

        // Send vector info to your own workers
        for (int i = 0; i < (int)cluster_workers.size(); i++) {
            int start = i * (double)balance_factors.at(0) / cluster_workers.size();
            int end = MIN((i + 1) * (double)balance_factors.at(0)/ cluster_workers.size(), balance_factors.at(0));

            std::vector<int> to_be_processed;
            for (int j = start; j < end; j++) {
                to_be_processed.push_back(source_vector[j]);
            }
            int to_be_processed_size = to_be_processed.size();
            send_info_vector(to_be_processed, &to_be_processed_size, cluster_workers[i], DATA_FLOW, 0);
        }

        // Receive from your workers
        for (const auto& worker : cluster_workers) {
            std::vector<int> recv_vec;
            recv_info_vector(recv_vec, rank, worker, DATA_FLOW, &recv_size);

            // Copy the contents into main array
            for (int j = 0; j < recv_size; j++) {
                final_vector.push_back(recv_vec[j]);
            }
        }
        // Receive data from COORD1 and COORD2
        reconstruct_vector(final_vector, rank, COORD2, MSG_FOR_0_FROM_1, &recv_size);
        reconstruct_vector(final_vector, rank, COORD2, MSG_FOR_0_FROM_2, &recv_size); 
    } 

    if (rank == COORD1) {
        // Receive vector to double from COORD0 through COORD2
        std::vector<int> recv_vector_cluster;
        recv_info_vector(recv_vector_cluster, rank, COORD2, MSG_FOR_1_FROM_0, &recv_size);

        // Send vector to your own workers
        send_to_workers(cluster_workers, recv_vector_cluster);

        // Receive doubled vector from workers
        std::vector<int> cluster_from_workers;
        recv_from_workers(cluster_workers, cluster_from_workers, rank);

        // Send back the doubled vector to COORD2
        int size_cluster_from_workers = cluster_from_workers.size();
        send_info_vector(cluster_from_workers, &size_cluster_from_workers, COORD2, MSG_FOR_0_FROM_1, 0);
    }

    if (rank == COORD2) {
        // Reroute info from COORD0 to COORD1
        std::vector<int> rerouting_vector;
        recv_info_vector(rerouting_vector, rank, COORD0, MSG_FOR_1_FROM_0, &recv_size);
        send_info_vector(rerouting_vector, &recv_size, COORD1, MSG_FOR_1_FROM_0, 0);

        // Receive vector from COORD0
        std::vector<int> recv_vector_cluster;
        recv_info_vector(recv_vector_cluster, rank, COORD0, MSG_FOR_2_FROM_0, &recv_size);

        // Send vector to workes
        send_to_workers(cluster_workers, recv_vector_cluster);

        // Receive vector from workers
        std::vector<int> cluster_from_workers;
        recv_from_workers(cluster_workers, cluster_from_workers, rank);
        int size_cluster_from_workers = cluster_from_workers.size();
        send_info_vector(cluster_from_workers, &size_cluster_from_workers, COORD0, MSG_FOR_0_FROM_2, 0);

        // Reroute info from COORD1 to COORD0
        recv_info_vector(rerouting_vector, rank, COORD1, MSG_FOR_0_FROM_1, &recv_size);
        send_info_vector(rerouting_vector, &recv_size, COORD0, MSG_FOR_0_FROM_1, 0);
    }

    if (rank > 2) {
        // Receive the data to be doubled from your coordinator
        std::vector<int> recv_v;
        recv_info_vector(recv_v, rank, my_coordinator, DATA_FLOW, &recv_size);
        // Double the vector
        for (int i = 0; i < recv_size; i++) {
            recv_v[i] <<= 1;
        }
        // Send back the doubld vector to your coordinator
        send_info_vector(recv_v, &recv_size, my_coordinator, DATA_FLOW, 0);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == COORD0) {
        print_result(final_vector);
    }
    MPI_Finalize();
    return 0;
}