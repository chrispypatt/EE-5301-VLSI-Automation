#include <math.h> 
#include <deque> 
#include <map>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

// #define K 0.001//Boltzman constant - NOW IN PLACEMENT.CPP
#define T_O 5000//Initial temperature
#define T_FREEZE 1 //Stopping temperature 


struct Coordinates {
	double left_x,right_x,y;//position of gate in a layout
	double wire_length;
};

struct layout_row {
	double curr_width = 0;	
	deque<node> row;

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
		starting_HPWL = copy.starting_HPWL;
		actual_width = copy.actual_width;
		width = copy.width;
		height = copy.height;
		rows = new layout_row[(int)height];
		gate_coordinates = copy.gate_coordinates;
		for (int i=0;i<(int)height;i++){
			rows[i]=copy.rows[i];
		}
	}
	double total_HPWL,starting_HPWL;
	double width, height;
	double actual_width;
	struct layout_row *rows; //this will be the chip containing rows of gates
	map<string, Coordinates> gate_coordinates;


	void gate_pushback(node n, double layout_y){
		gate_coordinates[n.key].left_x = rows[(int)layout_y].curr_width;
		rows[(int)layout_y].curr_width += n.width;
		gate_coordinates[n.key].right_x = rows[(int)layout_y].curr_width;
		gate_coordinates[n.key].y = layout_y;
		rows[(int)layout_y].row.push_back(n);
	}

	double get_cost(double T){
		return  0.7*actual_width*height + 0.3*total_HPWL;	
	}

	void destruct(){
		total_HPWL = 0;
		starting_HPWL = 0;
		width = 0;
		height = 0;
		gate_coordinates.clear();
		delete [] rows;
	}

	void calculate_wire_lengths();
	void recalculate_wire_lengths(int gate1_row, int gate1_column, int gate2_row, int gate2_column);
	double calculate_wire_length(node n);
};

class Chip {
	public:
	int NUM_MOVES_PER_STEP;
	Circuit circuit;
	struct Chip_Layout chip_layout; //this will be the chip gate holder

	void init_chip(Circuit circuit);
	void destroy_chip();
	void print_chip(double seconds);
	void write_chip(string in_file, double seconds);

	//functions for annealing
	bool accept_move(double delta_cost, double temp, double K);
	void simulated_annealing(string in_file);
	Chip_Layout perturb(Chip_Layout *curr_sol);
};

int random_int(int min, int max);
void printProgress (double percentage);
