#include <math.h> 
#include <deque> 

#define K 10.0 //Boltzman constant
#define T_O 100000.0 //Initial temperature
#define T_FREEZE 1.0 //Stopping temperature 

struct layout_row { 
	double curr_width = 0;	
	deque<node*> row;

	void gate_pushback(node *n, double layout_y){
		n->left_x = curr_width;
		n->right_x = curr_width + n->width;
		n->y = layout_y;
		row.push_back(n);
		curr_width += n->width;
	}

	node* gate_popback(){
		node* n = row.back();
		row.pop_back();
		curr_width -= n->width;
		return n;
	}
};

struct Chip_Layout {
	double total_HPWL;
	double width, height;
	struct layout_row *rows; //this will be the chip containing rows of gates

	void init_size(double h, double w){
		width = w;
		height = h;
		//set up our gate data structure.
		rows = new layout_row[(int)height];
	}

	void calculate_wire_lengths();

	double get_cost(){
		return total_HPWL;
	}
};

class Chip {
	public:
	int NUM_MOVES_PER_STEP;
	Circuit circuit;

	struct Chip_Layout chip_layout; //this will be the chip gate holder

	void init_chip(Circuit circuit);
	void destroy_chip();
	void print_chip();
	void write_chip(string in_file);
	void calculate_wire_lengths();

	//functions for annealing
	bool accept_move(double delta_cost, double temp);
	void simulated_annealing();
	Chip_Layout perturb(Chip_Layout curr_sol);
};

