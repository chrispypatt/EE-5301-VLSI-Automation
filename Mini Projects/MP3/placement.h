#include <math.h> 
#include <map>
#include <string>
#include <vector>
#include <limits>
#include <sys/stat.h> 
#include <random>
// #include <time.h>

using namespace std;

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

// #define K 0.001//Boltzman constant - NOW IN PLACEMENT.CPP
// #define T_O 5000//Initial temperature
// #define T_FREEZE 1 //Stopping temperature 

class Chip {
	public:
	int NUM_MOVES_PER_STEP;
	double height, width;
	Circuit circuit;
	vector <string> pos_loci, neg_loci;
	map <string,int> pos_index,neg_index;//indicies of each element in the loci
	Graph Gh, Gv; //Adjacency lists got vert and horz constraint graphs 
	Chip(){}
	Chip(Circuit ckt);

	void find_placement();

	// void destroy_chip();
	void print(double seconds);
	void write(string in_file, double seconds);

	//functions for annealing
	// bool accept_move(double delta_cost, double temp, double K);
	// void simulated_annealing(string in_file);
	// Chip_Layout perturb(Chip_Layout *curr_sol);
};

int random_int(int min, int max);
void printProgress (double percentage);
