#include <stdio.h>
#include <cstdlib>
#include <iostream> 
#include <string>
#include <queue> 
#include <map> 
#include <vector>
#include <limits>
#include <sys/stat.h> 
#include <sys/types.h> 
#include <random>
#include <algorithm>

#include <time.h>


using namespace std;

#include "parser.h"
#include "placement.h"
#include "helpers.h"


int main(int argc, char *argv[]){
	Circuit circuit;
	Chip chip;
	clock_t start, end;

	mkdir("output", 0777);
    //argc is number of args
    //argv is array of pointers to arguments
	if(argc == 2){
        //read_ckt
		string file_name = argv[1];
		cout << "-----------------------------------------------------" << endl;
		circuit.parseNetlist(file_name);

		//timer for initial placing and annealing
		start = clock();

		chip.init_chip(circuit);
		chip.simulated_annealing(file_name);

		end = clock(); 
		auto duration = ((float)end-(float)start)/CLOCKS_PER_SEC; 

		// double seconds = (double)duration.count()/1000000;

		chip.write_chip(file_name, duration);
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
	cout << "Placing gates on chip" << "." << "." << "." << endl;
	circuit = ckt; //save input circuit in our chip for future reference

	NUM_MOVES_PER_STEP = ckt.nodes_queue.size(); //Do # gates amount of moves per temp step

	//Make our grid layout
	double total_area = ckt.total_gates_area;
	double width = ceil(sqrt(total_area));
	double height = ceil(total_area/width);
	chip_layout = Chip_Layout(height,width);

	//setup vector of nodes for random placement
	vector<node*> nodes_r;
	for (auto elem: ckt.nodes) {
		node *n = elem.second;
		nodes_r.push_back(n);
	}
	default_random_engine  rng(time(NULL)); 
	shuffle(begin(nodes_r), std::end(nodes_r), rng);

	//fill layout randomly  with our gates
	while(!nodes_r.empty()){
		node *n = nodes_r.back();
		bool placed = false;

		//TODO: Faster way of initialization placeing
		//Keep track of row with least width in case node cant fit in any row.
		double min_width = chip_layout.width*width;
		int min_row;
		//place in next open spot of layout
		for (int i=0; i<chip_layout.height; i++){
			if ((chip_layout.rows[i].curr_width + n->width) < chip_layout.width){
				chip_layout.gate_pushback(*n,(double)i);
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
			chip_layout.gate_pushback(*n,(double)min_row);
		}
		nodes_r.pop_back();
	} 
	chip_layout.calculate_wire_lengths();
	chip_layout.starting_HPWL = chip_layout.total_HPWL;
}

/*//////////////////////END Chip Layout////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*					CHIP SIMULATED ANNEALING									  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void Chip::simulated_annealing(string in_file){
	string output_file;
	size_t found = in_file.find_last_of("/\\");
	output_file = split(in_file.substr(found+1), "."  )[0];
	ofstream newFile("output/intermediate_details_" + output_file + ".txt");

	//Initital solution is the initialized llayout
	struct Chip_Layout curr_sol;
	double delta_cost;
	int count = 0, zero_moves_count = 0;

	double cool_down_factor = 0.95;//T_O*.01;
	double T = T_O;

	//determine k - because input size, and thus, area and HPWL
	//grow with input netlist size, k should scale otherwise annealing 
	//acts differently from size to size
	double area = chip_layout.total_HPWL;
	int power = 0;
	while (area > 100){
		area /=10.0;
		power += 1;
	}
	double K = 0.001 * pow(10,power);

	curr_sol = chip_layout;

	cout << "Conducting Simulated Annealing" << "." << "." << "." << endl << endl;
	newFile <<"Iteration"<<"\t"  <<"HPWL" <<"\t"  <<"Cost"<< "\t" << "Moves" <<"\tT"<< endl;
	newFile<<"START" <<"\t"<< curr_sol.total_HPWL<<"\t"<< curr_sol.get_cost(T) << "\t" << 0.0<<"\t" << T << endl;
	
	//perform annealing!!
	while (T > T_FREEZE && zero_moves_count < 10){//allow for faster termination if consectively stagnent
		count++;
		int moves = 0;
		printProgress((double)count/207.0); //peace of mind to user. not always accurate
		for (int i = 0; i < NUM_MOVES_PER_STEP; i++){
			struct Chip_Layout next_sol = perturb(&curr_sol);
			delta_cost = next_sol.get_cost(T) - curr_sol.get_cost(T);
			if (accept_move(delta_cost, T, K)){
				moves++;
				curr_sol.destruct();//don't leak the memory
				curr_sol = next_sol;
			}else{
				next_sol.destruct();//don't leak the memory
			}
		}

		if (moves < 1){ //count up zero move iterations
			zero_moves_count++;//If too many 0's in a row, call the solution good to save time
		}else{
			zero_moves_count = 0;
		}

		newFile<<count <<"\t"<< curr_sol.total_HPWL<<"\t"<< curr_sol.get_cost(T) << "\t" << moves<<"\t" << T << endl;
		T *= cool_down_factor;
	}
	printProgress(1.0);
	cout << endl;
	chip_layout = curr_sol;
}


Chip_Layout Chip::perturb(Chip_Layout *curr_sol){
	Chip_Layout new_sol = *curr_sol;
	//get 2 random grid points to swap
	int i1 = random_int(0,(int)new_sol.height-1);
	int j1 = random_int(0,new_sol.rows[i1].row.size()-1);
	int i2 = random_int(0,(int)new_sol.height-1);
	int j2 = random_int(0,new_sol.rows[i2].row.size()-1);

	//swap gates
	deque<node> temp1, temp2;
	iter_swap(new_sol.rows[i1].row.begin() + j1, new_sol.rows[i2].row.begin() + j2);

	//clear our layout's two rows
	temp1.swap(new_sol.rows[i1].row);
	temp2.swap(new_sol.rows[i2].row);
	new_sol.rows[i1].curr_width = 0;
	new_sol.rows[i2].curr_width = 0;

	//redo placement of those rows to update x and y values
	for (int i = 0; i < temp1.size(); i++){
		new_sol.gate_pushback(temp1[i],i1);
	}
	for (int i = 0; i < temp2.size(); i++){
		new_sol.gate_pushback(temp2[i],i2);
	}

	new_sol.calculate_wire_lengths();
	return new_sol;
}


bool Chip::accept_move(double delta_cost, double temp, double K){
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
	// delete [] chip_layout.rows;
	chip_layout.rows = nullptr;
}

/*//////////////////////END SIMULATED ANNEALING////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*			CHIP PRINTOUT FUCNTIONS (TERMINAL AND FILE)							  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void Chip::print_chip(){
	cout << "-------------------Chip Info----------------------" << endl;
	cout << endl << "Bounding Chip area: " << chip_layout.width*chip_layout.height << endl;
	cout << "Bounding Chip dimensions: " << chip_layout.width << "x" << chip_layout.height << endl << endl;
	cout << endl << "Actual Chip area: " << chip_layout.actual_width*chip_layout.height << endl;
	cout << "Actual Chip dimensions: " << chip_layout.actual_width << "x" << chip_layout.height << endl << endl;
	cout << "Initial HPWL: " << chip_layout.starting_HPWL << endl;
	cout << "After Annealing HPWL: " << chip_layout.total_HPWL << endl;
	cout << "Annealing Execution Time: " << seconds << "s" << endl << endl;
	cout << "-------------------Gate Info----------------------" << endl;
	cout <node*> temp_queue = circuit.nodes_queue; //copy the original queue to the temporary queue
	while (!temp_queue.empty()){
		node *n = temp_queue.front();
		cout << n->name << ": ";
		cout << "x=" << chip_layout.gate_coordinates[n->key].left_x;
		cout << ", y=" << chip_layout.gate_coordinates[n->key].y;
		cout << endl;
		temp_queue.pop();
	} 
	cout << "-------------------Chip Layout----------------------" << endl;
	//print out chip for checking
	for (int i=0; i<chip_layout.height; i++){
		deque<node> row = chip_layout.rows[i].row;
		cout << endl << "------" << " Row " << i << " : width ";
		cout << chip_layout.rows[i].curr_width << " ------" << endl << "| ";
		while(!row.empty()){
			node temp = row.front();
			row.pop_front();
			cout << temp.name << " | ";
		}
		cout << endl << endl;
	}
	cout << "-----------------------------------------------------" << endl;
}

void Chip::write_chip(string in_file, double seconds){
	//create directory for output files
	string output_file;
	size_t found = in_file.find_last_of("/\\");
	output_file = split(in_file.substr(found+1), "."  )[0];

	ofstream newFile("output/chip_details_" + output_file + ".txt");
	if(!newFile.is_open()){
		cout << "Unable to open or create output file: " << "/output/chip_details_" << output_file << ".txt"<< endl;
		exit(1); 
	}else{//write out to both terminal and output file
		newFile << "-------------------Chip Info----------------------" << endl;
		newFile << endl << "Bounding Chip area: " << chip_layout.width*chip_layout.height << endl;
		newFile << "Bounding Chip dimensions: " << chip_layout.width << "x" << chip_layout.height << endl << endl;
		newFile << endl << "Actual Chip area: " << chip_layout.actual_width*chip_layout.height << endl;
		newFile << "Actual Chip dimensions: " << chip_layout.actual_width << "x" << chip_layout.height << endl << endl;
		newFile << "Initial HPWL: " << chip_layout.starting_HPWL << endl;
		newFile << "After Annealing HPWL: " << chip_layout.total_HPWL << endl;
		newFile << "Annealing Execution Time: " << seconds << "s" << endl << endl;
		newFile << "-------------------Gate Info----------------------" << endl;
		queue <node*> temp_queue = circuit.nodes_queue; //copy the original queue to the temporary queue
		while (!temp_queue.empty()){
			node *n = temp_queue.front();
			newFile << n->name << ": ";
			newFile << "x=" << chip_layout.gate_coordinates[n->key].left_x;
			newFile << ", y=" << chip_layout.gate_coordinates[n->key].y;
			newFile << endl;
			temp_queue.pop();
		} 
		newFile << "-------------------Chip Layout----------------------" << endl;
		//print out chip for checking
		for (int i=0; i<chip_layout.height; i++){
			deque<node> row = chip_layout.rows[i].row;
			newFile << endl << "------" << " Row " << i << " : width ";
			newFile << chip_layout.rows[i].curr_width << " ------" << endl << "| ";
			while(!row.empty()){
				node temp = row.front();
				row.pop_front();
				newFile << temp.name << " | ";
			}
			newFile << endl;
		}
		newFile << "-----------------------------------------------------" << endl;
	}
	cout << endl << "Output has been generated and can be found at: " << endl;
	cout << "output/chip_details_" + output_file + ".txt" << endl;
	cout << "	&" << endl;
	cout << "output/intermediate_details_" + output_file + ".txt" << endl << endl;

}

/*//////////////////////END PRINTOUT////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*						Chip Layout Functions									  */
/*						for updating wire lengths.								  */
/*					Current method is to traverse entire chip. 					  */
/*////////////////////////////////////////////////////////////////////////////////*/

void Chip_Layout::calculate_wire_lengths(){
	total_HPWL = 0;
	actual_width = 0;
	for (int i=0; i<height; i++){
		deque<node> row = rows[i].row;
		if (rows[i].curr_width > actual_width){
			actual_width = rows[i].curr_width;
		}
		while(!row.empty()){
			node gate = row.front();
			row.pop_front();
			//look at all outputs of this node. 
			gate_coordinates[gate.key].wire_length = calculate_wire_length(gate);
			total_HPWL += gate_coordinates[gate.key].wire_length ;
		}
	}
}

double Chip_Layout::calculate_wire_length(node n){
	double length = 0;
	struct Coordinates coord = gate_coordinates[n.key];

	//set extreme values so we can compare to our net
	double min_x = coord.left_x, min_y = coord.y;
	double max_x = coord.left_x, max_y = coord.y;

	//iterate through all output gates (net)
	for (auto elem: n.outputs) {
		double x = gate_coordinates[elem.second->key].left_x;
		double y = gate_coordinates[elem.second->key].y;
		if (x > max_x){
			max_x = x;
		}
		if (x < min_x){
			min_x = x;
		}
		if (y > max_y){
			max_y = y;
		}
		if (y < min_y){
			min_y = y;
		} 
	}
	//we have max and mins, now calculate HPWL
	//(2*length of rectangle + 2* height of rectangle)/2
	//or length of rectangle + height of rectangle
	length = (max_x-min_x)+(max_y-min_y);
	// gate_coordinates[n.key].wire_length = length;
	// cout << length << endl;
	return length;
}

/*//////////////////////END Chip Layout////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*			Helper functions for minor but important things						  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

int random_int(int min, int max){
	static bool first = true;
	if (first) {  
		srand(time(NULL)); //seeding for the first time only!
		first = false;
	}
	return min + rand() % (( max + 1 ) - min);
}




//From razzak on https://stackoverflow.com/questions/14539867/how-to-display-a-progress-indicator-in-pure-c-c-cout-printf
void printProgress (double percentage){
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf ("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush (stdout);
}

/*//////////////////////END HELPERS////////////////////////////////////////////*/


