#include <stdio.h>
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
		chip.calculate_wire_lengths();
		// chip.print_chip(file_name);
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
	//Make a square for our chip of size sqrt(gate_area_totals)xsqrt(gate_area_totals)
	width = ceil(sqrt(ckt.total_gates_area));
	height = width;

	//set up our gate data structure. Array of linked lists.
	layout = new layout_row[(int)height];

	//fill layout with our gates
	for (auto elem: ckt.nodes) {
		node *n = elem.second;
		bool placed = false;

		//TODO: Faster way of initialization placeing
		//Keep track of row with least width in case node cant fit in any row.
		double min_width = width*width;
		int min_row;
		//place in next open spot of layout
		for (int i=0; i<height; i++){
			if ((layout[i].curr_width + n->width) < width){
				layout[i].gate_pushback(n,(double)i);
				placed = true;
				break;
			}
			if (layout[i].curr_width < min_width){
				min_width = layout[i].curr_width;
				min_row = i;
			}
		}
		if (!placed){//no more room on chip. We will create an almost square of nodes
			//put this poor orphan in the shortest row
			layout[min_row].gate_pushback(n,(double)min_row);
		}
	} 
}

void Chip::calculate_wire_lengths(){
	total_HPWL = 0;
	for (int i=0; i<height; i++){
		deque<node*> row = layout[i].row;
		while(!row.empty()){
			node *gate = row.front();
			row.pop_front();
			//look at all outputs of this node. 
			gate->wire_length = gate->calculate_wire_length();
			total_HPWL += gate->wire_length;
		}
	}
}

void Chip::destroy_chip(){
	width = 0;
	height = 0;
	delete [] layout;
	layout = nullptr;
}

void Chip::print_chip(){
	cout << "-------------------Chip Info----------------------" << endl;
	cout << endl << "Chip area: " << width*height << endl;
	cout << "Chip dimensions: " << width << "x" << height << endl;
	cout << "Total HPWL: " << total_HPWL << endl << endl;
	cout << "-------------------Chip Layout----------------------" << endl;
	//print out chip for checking
	for (int i=0; i<height; i++){
		deque<node*> row = layout[i].row;
		cout << endl << "------" << " Row " << i << " : width ";
		cout << layout[i].curr_width << " ------" << endl << "| ";
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
		newFile << endl << "Chip area: " << width*height << endl;
		newFile << "Chip dimensions: " << width << "x" << height << endl;
		newFile << "Total HPWL: " << total_HPWL << endl << endl;
		newFile << "-------------------Chip Layout----------------------" << endl;
		//print out chip for checking
		for (int i=0; i<height; i++){
			deque<node*> row = layout[i].row;
			newFile << endl << "------" << " Row " << i << " : width ";
			newFile << layout[i].curr_width << " ------" << endl << "| ";
			while(!row.empty()){
				node *temp = row.front();
				row.pop_front();
				newFile << temp->name << " | ";
			}
			newFile << endl << endl;
		}
		newFile << "-----------------------------------------------------" << endl;
	}
	cout << "Output has been generated and can be found at: " << "output/chip_details_" + output_file + ".txt" << endl << endl;
}

