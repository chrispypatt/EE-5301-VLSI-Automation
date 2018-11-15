#include <math.h> 
#include <deque> 
#include <map>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

// #define K 0.001//Boltzman constant
#define T_O 40000//Initial temperature
#define T_FREEZE 1 //Stopping temperature 


struct Coordinates {
	double left_x,right_x,y;//position of gate in a layout
	double wire_length;
};

//shouldnt be here but whatever haha
//should be in chip but idk how to do it that way
map<string, Coordinates> gate_coordinates;


struct layout_row {
	double curr_width = 0;	
	deque<node> row;

	void gate_pushback(node n, double layout_y){
		gate_coordinates[n.key].left_x = curr_width;
		gate_coordinates[n.key].right_x = curr_width + n.width;
		gate_coordinates[n.key].y = layout_y;
		row.push_back(n);
		curr_width += n.width;
	}

	node gate_popback(){
		node n = row.back();
		row.pop_back();
		curr_width -= n.width;

		//This is invalid but these should not stay this way long.
		gate_coordinates[n.key].left_x = -1;
		gate_coordinates[n.key].right_x = -1;
		gate_coordinates[n.key].y = -1;

		return n;
	}

	void clear(){
		row.clear();
		curr_width = 0;
	}

};

struct Chip_Layout {
	Chip_Layout(){}
	Chip_Layout(double h, double w){ 
		width = w;
		height = h;
		//set up our gate data structure.
		rows = new layout_row[(int)height];
	}
    Chip_Layout(const Chip_Layout& copy){ 
		total_HPWL = copy.total_HPWL;
		width = copy.width;
		height = copy.height;
		rows = new layout_row[(int)height];
		for (int i=0;i<(int)height;i++){
			rows[i]=copy.rows[i];
		}
	}
	double total_HPWL;
	double width, height;
	double actual_width;
	// double actual_width,actual_height;
	struct layout_row *rows; //this will be the chip containing rows of gates

	void calculate_wire_lengths();

	double get_cost(double T){
		return  0.7*actual_width*height + 0.3*total_HPWL;	
	}

	double calculate_wire_length(node n);
};

class Chip {
	public:
	int NUM_MOVES_PER_STEP;
	Circuit circuit;
	
	struct Chip_Layout chip_layout; //this will be the chip gate holder

	void init_chip(Circuit circuit);
	void destroy_chip();
	void print_chip();
	void write_chip(string in_file, double seconds);
	void calculate_wire_lengths();

	//functions for annealing
	bool accept_move(double delta_cost, double temp, double K);
	void simulated_annealing(string in_file);
	Chip_Layout perturb(Chip_Layout curr_sol);
};

int random_int(int min, int max);
void printProgress (double percentage);
