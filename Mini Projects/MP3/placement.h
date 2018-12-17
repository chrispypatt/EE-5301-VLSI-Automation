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
#define T_O 500//Initial temperature
#define T_FREEZE 1 //Stopping temperature

struct Coordinates {
	double x,y;//position of gate in a layout
};

class Placement{
	public:
	double height, width, HPWL;
	vector <string> pos_loci, neg_loci;
	map <string,int> pos_index,neg_index;//indicies of each element in the loci
	map <string,Coordinates> module_coordinates; 
	Graph Gh, Gv; //Adjacency lists of vert and horz constraint graphs 
	
	Placement(){
	}
	Placement(const Placement& copy){ 
		HPWL = copy.HPWL;
		height = copy.height;
		width = copy.width;
		pos_loci = copy.pos_loci;
		neg_loci = copy.neg_loci;
		pos_index = copy.pos_index;
		neg_index = copy.neg_index;
	}

	double get_cost(string option);
	void find_placement(Circuit circuit);
	void update_placement(Circuit circuit);
	void calculate_wire_lengths(vector<HyperEdge> edges);

	void destruct(){
		HPWL = 0;
		width = 0;
		height = 0;
		pos_index.clear();
		neg_index.clear();
		module_coordinates.clear();
		pos_loci.clear();
		neg_loci.clear();
		Gh.destruct();
		Gv.destruct();
	}
};

class Chip {
	public:
	int NUM_MOVES_PER_STEP;
	Placement curr_placement;
	Circuit circuit;

	double K;

	double s_area, s_hpwl;

	Chip(){}
	Chip(Circuit ckt);


	// void destroy_chip();
	void print(double seconds);
	void write(string in_file, double seconds, string option);

	//functions for annealing
	bool accept_move(double delta_cost, double temp, double K);
	void annealing_init(string option);
	void simulated_annealing(string in_file, string option);
	Placement perturb(Placement *curr_sol);
};

int random_int(int min, int max);
void printProgress (double percentage);
