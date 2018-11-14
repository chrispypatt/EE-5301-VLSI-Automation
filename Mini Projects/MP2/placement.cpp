#include <stdio.h>
#include <cstdlib>
#include <iostream> 
#include <string>
#include <queue> 
#include <map> 
#include <limits>
#include <sys/stat.h> 
#include <sys/types.h> 

using namespace std;

#include "parser.h"
#include "placement.h"
#include "helpers.h"

int main(int argc, char *argv[]){
	Circuit circuit;
	Chip chip;
    //argc is number of args
    //argv is array of pointers to arguments
	if(argc == 2){
        //read_ckt
		string file_name = argv[1];
		cout << "-----------------------------------------------------" << endl;
		cout << endl << "Parsing file: " << file_name << endl;
		circuit.parseNetlist(file_name);
		cout << "Placing gates on chip" << "." << "." << "." << endl;
		chip.init_chip(circuit);

		//perturb a chip to see if it messes anything up
		Chip chip2 = chip;
		node* n1 = chip2.chip_layout.rows[1].gate_popback();
		node* n2 = chip2.chip_layout.rows[2].gate_popback();
		chip2.chip_layout.rows[1].gate_pushback(n2,1);
		chip2.chip_layout.rows[2].gate_pushback(n1,2);



		chip.chip_layout.calculate_wire_lengths();
		chip2.chip_layout.calculate_wire_lengths();

		chip.write_chip(file_name);
		cout << "-----------------------------------------------------" << endl;
    }else{
		cerr << endl;
		cerr << "Usage: " << argv[0] << " <bench_file>" << endl;
		cerr << endl;
		return 1;
	}

    return(0);
}

void Chip::init_chip(Circuit ckt){
	circuit = ckt; //save input circuit in our chip for future reference

	NUM_MOVES_PER_STEP = ckt.nodes_queue.size(); //Do # gates amount of moves per temp step

	//Make our layout grid
	double total_area = ckt.total_gates_area;
	double width = ceil(sqrt(total_area));
	double height = ceil(total_area/width);
	chip_layout.init_size(height,width);

	//fill layout with our gates
	for (auto elem: ckt.nodes) {
		node *n = elem.second;
		bool placed = false;

		//TODO: Faster way of initialization placeing
		//Keep track of row with least width in case node cant fit in any row.
		double min_width = chip_layout.width*width;
		int min_row;
		//place in next open spot of layout
		for (int i=0; i<chip_layout.height; i++){
			if ((chip_layout.rows[i].curr_width + n->width) < chip_layout.width){
				chip_layout.rows[i].gate_pushback(n,(double)i);
				placed = true;
				break;
			}
			if (chip_layout.rows[i].curr_width < min_width){
				min_width = chip_layout.rows[i].curr_width;
				min_row = i;
			}
		}
		if (!placed){//no more room on chip. We will create an almost square of nodes
			//put this poor orphan in the shortest row
			chip_layout.rows[min_row].gate_pushback(n,(double)min_row);
		}
	} 
}

void Chip_Layout::calculate_wire_lengths(){
	total_HPWL = 0;
	for (int i=0; i<height; i++){
		deque<node*> row = rows[i].row;
		while(!row.empty()){
			node *gate = row.front();
			row.pop_front();
			//look at all outputs of this node. 
			gate->wire_length = gate->calculate_wire_length();
			total_HPWL += gate->wire_length;
		}
	}
}

void Chip::simulated_annealing(){
	//Initital solution is layout
	struct Chip_Layout curr_sol, next_sol;
	double T = T_O;
	curr_sol = chip_layout;
	double delta_cost;
	while (T > T_FREEZE){
		for (int i = 0; i < NUM_MOVES_PER_STEP; i++){
			next_sol = perturb(curr_sol);
			delta_cost = next_sol.get_cost() - curr_sol.get_cost();
			if (accept_move(delta_cost, T)){
				curr_sol = next_sol;
			}
		}
		T = T/10;
	}
	chip_layout = curr_sol;
	cout << chip_layout.total_HPWL << endl;
}


void Chip::perturb(Chip_Layout curr_sol){
	//make changes, store previous state
	Chip_Layout new_sol = curr_sol;
	//perturb here
	return new_sol;
}




bool Chip::accept_move(double delta_cost, double temp){
	if (delta_cost < 0) { return true; }//we'd be crazy 🤪 not to accept this!!
	//if change in cost is not negative, use Boltzman probability to decide to accept
	double boltz = exp((-1)*(delta_cost/(K*temp)));
	double r = (double)rand()/RAND_MAX; //# between 0 and 1
	//accept or reject
	if (r < boltz) { 
		return true;
	}
	return false;
}

void Chip::destroy_chip(){
	chip_layout.width = 0;
	chip_layout.height = 0;
	delete [] chip_layout.rows;
	chip_layout.rows = nullptr;
}

void Chip::print_chip(){
	cout << "-------------------Chip Info----------------------" << endl;
	cout << endl << "Chip area: " << chip_layout.width*chip_layout.height << endl;
	cout << "Chip dimensions: " << chip_layout.width << "x" << chip_layout.height << endl;
	cout << "Total HPWL: " << chip_layout.total_HPWL << endl << endl;
	cout << "-------------------Chip Layout----------------------" << endl;
	//print out chip for checking
	for (int i=0; i<chip_layout.height; i++){
		deque<node*> row = chip_layout.rows[i].row;
		cout << endl << "------" << " Row " << i << " : width ";
		cout << chip_layout.rows[i].curr_width << " ------" << endl << "| ";
		while(!row.empty()){
			node *temp = row.front();
			row.pop_front();
			cout << temp->name << " | ";
		}
		cout << endl << endl;
	}
	cout << "-----------------------------------------------------" << endl;
}

void Chip::write_chip(string in_file){
	//create directory for output files
	string output_file;
	size_t found = in_file.find_last_of("/\\");
	output_file = split(in_file.substr(found+1), "."  )[0];

    mkdir("output", 0777);
	ofstream newFile("output/chip_details_" + output_file + ".txt");
	if(!newFile.is_open()){
		cout << "Unable to open or create output file: " << "/output/chip_details_" << output_file << ".txt"<< endl;
		exit(1); 
	}else{//write out to both terminal and output file
		newFile << "-------------------Chip Info----------------------" << endl;
		newFile << endl << "Chip area: " << chip_layout.width*chip_layout.height << endl;
		newFile << "Chip dimensions: " << chip_layout.width << "x" << chip_layout.height << endl;
		newFile << "Total HPWL: " << chip_layout.total_HPWL << endl << endl;
		newFile << "-------------------Chip Layout----------------------" << endl;
		//print out chip for checking
		for (int i=0; i<chip_layout.height; i++){
			deque<node*> row = chip_layout.rows[i].row;
			newFile << endl << "------" << " Row " << i << " : width ";
			newFile << chip_layout.rows[i].curr_width << " ------" << endl << "| ";
			while(!row.empty()){
				node *temp = row.front();
				row.pop_front();
				newFile << temp->name << " x:y " << temp->left_x << ":" << temp->y << " | ";
			}
			newFile << endl << endl;
		}
		newFile << "-----------------------------------------------------" << endl;
	}
	cout << "Output has been generated and can be found at: " << "output/chip_details_" + output_file + ".txt" << endl << endl;
}

