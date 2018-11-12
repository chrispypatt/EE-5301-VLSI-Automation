#include <math.h> 
#include <deque> 

struct layout_row { //row as a linked list
	double curr_width = 0;	
	deque<node*> row;

	void gate_pushback(node *n, double layout_y){
		n->left_x = curr_width;
		n->right_x = curr_width + n->width;
		n->y = layout_y;
		row.push_back(n);

		curr_width += n->width;
	}
};

class Chip {
	public:
	double width, height;
	double total_HPWL;

	struct layout_row *layout; //this will be the chip containing rows of gates
	void init_chip(Circuit circuit);
	void destroy_chip();
	void print_chip();
	void write_chip(string in_file);
	void calculate_wire_lengths();
};

