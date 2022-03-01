#include "tema3_utils.h"

// Reads cluster info from ".txt" files
void read_input(std::vector<int>& cluster_workers, int rank) {
    std::string file_name = "cluster";
    file_name += (std::to_string(rank) + ".txt");
    int aux;

    std::ifstream inputFile(file_name);
    if (inputFile) {
        int worker;
        inputFile >> aux;
        while (inputFile >> worker) {
            cluster_workers.push_back(worker);
        }
    }
    inputFile.close();
}

// Logs a message at the receiving end
void log_message(MPI_Status status, int rank) {
    std::cout << "M(" << status.MPI_SOURCE << ',' << rank << ')' << std::endl;
}

// Prints the full topology
void print_topology(Topology topologie, int rank) {
    std::string topologie_string = std::to_string(rank) + " ->";

    std::ostringstream out1;
    if (!topologie.clusters[0].empty())
    {
        std::copy(topologie.clusters[0].begin(), topologie.clusters[0].end() - 1, std::ostream_iterator<int>(out1, ","));
        out1 << topologie.clusters[0].back();
    }

    std::string result1 = " 0:" +  out1.str();

    std::ostringstream out2;
    if (!topologie.clusters[1].empty())
    {
        std::copy(topologie.clusters[1].begin(), topologie.clusters[1].end() - 1, std::ostream_iterator<int>(out2, ","));
        out2 << topologie.clusters[1].back();
    }

    std::string result2 = " 1:" +  out2.str();

    std::ostringstream out3;
    if (!topologie.clusters[2].empty())
    {
        std::copy(topologie.clusters[2].begin(), topologie.clusters[2].end() - 1, std::ostream_iterator<int>(out3, ","));
        out3 << topologie.clusters[2].back();
    }

    std::string result3 = " 2:" +  out3.str();
    
    std::string final = topologie_string + result1 + result2 + result3;
    std::cout << final << std::endl;
}

// Calculates the balance factors for each cluster so we can know how many elements
// should be send to each coordinator
void calculate_balance(std::vector<int>& factors, Topology topologie, int vector_size) {
    int total = topologie.clusters[0].size() + topologie.clusters[1].size() + 
                topologie.clusters[2].size();

    double fraction = (double)vector_size / total;
    int p1 = ceil(fraction * topologie.clusters[0].size());
    int p2 = ceil(fraction * topologie.clusters[1].size());
    int p3 = vector_size - p1 - p2;
    factors.push_back(p1);
    factors.push_back(p2);
    factors.push_back(p3);
}

// Print the doubled vector
void print_result(std::vector<int>& result) {
    std::cout << "Rezultat: ";
    for (const auto& x : result) {
        std::cout << x << ' ';
    }
    std::cout << std::endl;
}

// Split vector and send each chunk to its assigned worker
void send_to_workers(std::vector<int>& cluster_workers, std::vector<int>& recv_vector_cluster) {
    int recv_size = recv_vector_cluster.size();
    for (int i = 0; i < (int)cluster_workers.size(); i++) {
            int start = i * (double)recv_size/ cluster_workers.size();
            int end = MIN((i + 1) * (double)recv_size/ cluster_workers.size(), recv_size);

            std::vector<int> to_be_processed;
            for (int j = start; j < end; j++) {
                to_be_processed.push_back(recv_vector_cluster[j]);
            }

            int to_be_processed_size = to_be_processed.size();
            MPI_Send(&to_be_processed_size, 1, MPI_INT, cluster_workers[i], DATA_FLOW, MPI_COMM_WORLD);
            MPI_Send(&to_be_processed[0], to_be_processed_size, MPI_INT, cluster_workers[i], DATA_FLOW, MPI_COMM_WORLD);
        }
}

// Receive vector data from workers
void recv_from_workers(std::vector<int>& cluster_workers, std::vector<int>& cluster_from_workers, int rank) {
    MPI_Status recv_status;
    int recv_size;
    for (const auto& worker : cluster_workers) {
        std::vector<int> recv_vec;
        MPI_Recv(&recv_size, 1, MPI_INT, worker, DATA_FLOW, MPI_COMM_WORLD, &recv_status);
        log_message(recv_status, rank); // log the message first

        recv_vec.resize(recv_size);

        MPI_Recv(&recv_vec[0], recv_size, MPI_INT, worker, DATA_FLOW, MPI_COMM_WORLD, &recv_status);
        log_message(recv_status, rank); // log the message first

        // Copy the contents into main array
        for (int j = 0; j < recv_size; j++) {
            cluster_from_workers.push_back(recv_vec[j]);
        }
    }
}

// Receive vector data from coordinator
void recv_info_vector(std::vector<int>& recv_vector, int rank, int source, int tag, int* recv_size) {
    MPI_Status recv_status;
    MPI_Recv(recv_size, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &recv_status);
    log_message(recv_status, rank); // log the message first

    recv_vector.resize(*recv_size);

    MPI_Recv(&recv_vector[0], *recv_size, MPI_INT, source, tag, MPI_COMM_WORLD, &recv_status);
    log_message(recv_status, rank); // log the message first
}

// Send vector data to coordinator
void send_info_vector(std::vector<int>& send_vector, int *size, int dest, int tag, int offset) {
    MPI_Send(size, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
    MPI_Send(&send_vector[offset], *size, MPI_INT, dest, tag, MPI_COMM_WORLD);
}

// Generate the vector to be doubled
void init_vector(std::vector<int>& source_vector, int vector_size) {
    for (int i = 0; i < vector_size; i++) {
        source_vector.push_back(i);
    }
}

// Get the vector data from a coordinator
void reconstruct_vector(std::vector<int>& dest_vector, int rank, int source, int tag, int* recv_size) {
    std::vector<int>aux;
    recv_info_vector(aux, rank, source, tag, recv_size);

    for (int j = 0; j < *recv_size; j++) {
        dest_vector.push_back(aux[j]);
    }
}