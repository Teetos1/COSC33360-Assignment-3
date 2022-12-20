#include <cmath>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <map>
#include <fcntl.h>

struct args {
  char symbol;
  double prob;
  double cumux;
  pthread_mutex_t* mu_one;
  pthread_mutex_t* mu_two;
  pthread_cond_t* cond;
  int* parIdx;
  int idx;
};

//Function MakeBinary gets an individual symbol and calculates the fbar.
//Calculates the total of 1's and 0's each symbol has.
//Converts the fbar to binary in respect to the length.
std::string MakeBinary(std::string binary, double prob, double cumux)
{
    double bar;
    int total = 0;
    bar = (cumux - prob) + prob / 2;
    total = ceil(log2(1 / prob)) + 1;

    for(int i = 0; i < total; i++) {
        binary += '0';
    }

    int x;
    double j;
    for(x = 0, j = 0.5; x < binary.length(); x++, j /= 2) {
        if(j > bar)
            continue;
        binary[x] = '1';
        bar -= j;
    }
    
    return binary;
}

void* cum(void* arguments){
    args* global_args = (args*) arguments;

    //Copy variables from the struct into local variables.
    //It changes for the other threads.
    int local_thread_id = global_args->idx;
    char local_symbol = global_args->symbol;
    double local_prob = global_args->prob;
    double local_cumux = global_args->cumux;
    
    pthread_mutex_unlock(global_args->mu_one);

    std::string binary;
    //Calculates the binary of the symbol
    binary = MakeBinary(binary, local_prob, local_cumux);

    //wait condition to allow work in converting it to binary.
    //Forces an order of operation to occur.
    pthread_mutex_lock(global_args->mu_two);
    while((*global_args->parIdx) != local_thread_id){
        pthread_cond_wait(global_args->cond, global_args->mu_two);
    }
    pthread_mutex_unlock(global_args->mu_two);

    std::cout << "Symbol " << local_symbol << ", " << "Code: " << binary << std::endl;
    
    pthread_mutex_lock(global_args->mu_two);
    (*global_args->parIdx) = (*global_args->parIdx) + 1;
    pthread_cond_broadcast(global_args->cond);
    pthread_mutex_unlock(global_args->mu_two);

    return NULL;
}

int main() {
    //Map is used to store the frequency for each letter
    std::string characters;
    getline(std::cin, characters);
    std::map<char,int> map;
    for(auto i: characters){
        map[i]++;
    }

    //gets the size of the map, allows me to make the size of the struct
    int total = map.size();

    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_mutex_t mutex_two = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t turn = PTHREAD_COND_INITIALIZER;
    int parIdx = 0;

    args threadArgs;
    threadArgs.mu_one = &mutex;
    threadArgs.mu_two = &mutex_two;
    threadArgs.cond = &turn;
    threadArgs.parIdx = &parIdx;
    pthread_t threads[total];

    char symbol[total];
    double prob[total], cumux[total];
    double probability;
    int count = 0;
    double tempf;
    for (auto it = map.begin(); it != map.end(); it++) {
        symbol[count] = it->first;
        probability = it->second;
        probability = probability / characters.length();
        prob[count] = probability;
        tempf += prob[count];
        cumux[count] = tempf;
        count++;
    }

    std::cout << "SHANNON-FANO-ELIAS Codes:" << std::endl << std::endl;

    for(int i = 0; i < total; i++){
        pthread_mutex_lock(&mutex);
        threadArgs.symbol = symbol[i];
        threadArgs.prob = prob[i];
        threadArgs.cumux = cumux[i];
        threadArgs.idx = i;

        if(pthread_create(&threads[i], NULL, cum, &threadArgs)){
            std::cout << "Failed to create thread" << std::endl;
        }
    }
    for(int i = 0; i < total; i++){
        pthread_join(threads[i], NULL);
    }


    return 0;
}
