#include "parser.h"
#include "helpers.h"
#include "graph.h"
#include "placement.h"

int main(int argc, char *argv[]){
	Circuit circuit;
	Chip chip;
	// clock_t start, end;

	string file_name = argv[1];
	circuit.parseNetlist(file_name);

	//initialize netlist placement on chip
	chip = Chip(circuit);

	chip.write(file_name,0.0);

	// mkdir("output", 0777);
    //argc is number of args
    //argv is array of pointers to arguments
	// if(argc == 2){
    //     //read_ckt
	// 	string file_name = argv[1];
	// 	cout << "-----------------------------------------------------" << endl;
	// 	circuit.parseNetlist(file_name);

	// 	//timer for initial placing and annealing
	// 	start = clock();

	// 	chip.init_chip(circuit);
	// 	chip.simulated_annealing(file_name);

	// 	end = clock(); 
	// 	auto duration = ((float)end-(float)start)/CLOCKS_PER_SEC; 


	// 	chip.write_chip(file_name, duration);
	// 	cout << "-----------------------------------------------------" << endl;
    // }else if (argc == 3 && (strcmp(argv[1],"init_test") == 0)){
	// 	string file_name = argv[2];
	// 	cout << "-----------------------------------------------------" << endl;
	// 	circuit.parseNetlist(file_name);

	// 	chip.init_chip(circuit);

	// 	chip.write_chip(file_name, -1);
	// 	cout << "-----------------------------------------------------" << endl;
	// }else{
	// 	cerr << endl;
	// 	cerr << "Usage: " << argv[0] << " <bench_file>" << endl;
	// 	cerr << "		" << "or" << endl;
	// 	cerr << "	" << argv[0] << " init_test  <bench_file>" << endl;
	// 	cerr << endl;
	// 	return 1;
	// }

    return(0);
}

Chip::Chip(Circuit ckt){
	cout << "Placing modules on chip" << "..." << endl;
	circuit = ckt; //save input circuit in our chip for future reference

	NUM_MOVES_PER_STEP = ckt.module_keys.size(); //Do # modules amount of moves per temp step

	// //setup vector of module keys for random placement
	vector<string> module_rp, module_rn;
	for (auto elem: ckt.modules) {
		string key = elem.first;
		module_rp.push_back(key);
		module_rn.push_back(key);
	}

	// default_random_engine rng(time(NULL)); 
	default_random_engine rng(0);//use this for reapeatability
	shuffle(begin(module_rp), std::end(module_rp), rng);
	shuffle(begin(module_rn), std::end(module_rn), rng);

	//initalize data structures
	Gh = Graph(ckt.module_keys.size());
	Gv = Graph(ckt.module_keys.size());

	int index = 0;
	while(!module_rp.empty()){
		string key_p = module_rp.back();
		string key_n = module_rn.back();

		//set up positive and negative loci
		pos_loci.push_back(key_p);
		neg_loci.push_back(key_n);
		pos_index[key_p] = index;
		neg_index[key_n] = index;
		Gv.has_incoming[key_p] = false;
		Gh.has_incoming[key_p] = false;

		index++;
		module_rp.pop_back();
		module_rn.pop_back();
	} 

	find_placement();

	// chip_layout.calculate_wire_lengths();
	// chip_layout.starting_HPWL = chip_layout.total_HPWL;
	cout << "Initial placement completed!" << endl << endl;
}


void Chip::find_placement(){
	height = 0;
	width = 0;
	for(int i = 0; i < pos_loci.size(); i++){ //get A in positive loci
		for(int j = i+1; j < pos_loci.size(); j++){//get B in positive loci
			//check the order of these two locus in the negative loci vector
			// found (…A…B…, …A…B…), add wB weight from A->B to Gh
			if (neg_index[pos_loci[i]] < neg_index[pos_loci[j]]){
				//weight = xb-xa
				double weight = circuit.modules[pos_loci[i]]->width;
				Gh.has_incoming[pos_loci[j]] = true;
				Gh.addEdge(pos_loci[i],pos_loci[j], weight);
			}else{// found (…A…B…, …B…A…), add hB weight from B->A to Gv
				//weight = ya-yb
				double weight = circuit.modules[pos_loci[j]]->height;
				Gv.has_incoming[pos_loci[i]] = true;
				Gv.addEdge(pos_loci[j],pos_loci[i], weight);
			}
		}
	}

	//add edge with weight 0 from s to all nodes without an incoming edge
	for (auto elem: Gv.has_incoming){
		if (!elem.second){
			Gv.addEdge("S", elem.first, 0.0);
		}
	}
	for (auto elem: Gh.has_incoming){
		if (!elem.second){
			Gh.addEdge("S", elem.first, 0.0);
		}
	}

	//run our longest path algorithm to get x and y positions
	map<string,double> y_positions = Gv.longestPath("S");
	map<string,double> x_positions = Gh.longestPath("S");

	//record the x and y positions on our chip
	for (auto elem: y_positions){
		if (elem.first != "S"){
			//bottom edge calculation
			double y = elem.second ;
			circuit.modules[elem.first]->y_bottom = y;
			if (height < y + circuit.modules[elem.first]->height){ //find furthest top edge
				height = y + circuit.modules[elem.first]->height;
			}
		}
	}
	for (auto elem: x_positions){
		if (elem.first != "S"){
			//left edge calculation
			double x = elem.second;
			circuit.modules[elem.first]->x_left = x;
			if (width < x + circuit.modules[elem.first]->width){ //find furthest right edge
				width = x+ circuit.modules[elem.first]->width;
			}
		}
	}
}

/*//////////////////////END Chip Layout////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*					CHIP SIMULATED ANNEALING									  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

// void Chip::simulated_annealing(string in_file){
// 	string output_file;
// 	size_t found = in_file.find_last_of("/\\");
// 	output_file = split(in_file.substr(found+1), "."  )[0];
// 	ofstream newFile("output/intermediate_details_" + output_file + ".txt");

// 	//Initital solution is the initialized llayout
// 	struct Chip_Layout curr_sol;
// 	double delta_cost;
// 	int count = 0, zero_moves_count = 0;

// 	double cool_down_factor = 0.95;//T_O*.01;
// 	double T = T_O;

// 	//determine k - because input size, and thus, area and HPWL
// 	//grow with input netlist size, k should scale otherwise annealing 
// 	//acts differently from size to size
// 	double area = chip_layout.total_HPWL;
// 	int power = 0;
// 	while (area > 100){
// 		area /=10.0;
// 		power += 1;
// 	}
// 	double K = 0.001 * pow(10,power);

// 	curr_sol = chip_layout;

// 	cout << "Conducting Simulated Annealing" << "." << "." << "." << endl << endl;
// 	newFile <<"Iteration"<<"\t"  <<"HPWL" <<"\t"  <<"Cost"<< "\t" << "Moves" <<"\tT"<< endl;
// 	newFile<<"START" <<"\t"<< curr_sol.total_HPWL<<"\t"<< curr_sol.get_cost(T) << "\t" << 0.0<<"\t" << T << endl;
	
// 	//perform annealing!!
// 	while (T > T_FREEZE && zero_moves_count < 10){//allow for faster termination if consectively stagnent
// 		count++;
// 		int moves = 0;
// 		printProgress((double)count/167.0); //peace of mind to user. not always accurate
// 		for (int i = 0; i < NUM_MOVES_PER_STEP; i++){
// 			struct Chip_Layout next_sol = perturb(&curr_sol);
// 			delta_cost = next_sol.get_cost(T) - curr_sol.get_cost(T);
// 			if (accept_move(delta_cost, T, K)){
// 				moves++;
// 				curr_sol.destruct();//don't leak the memory
// 				curr_sol = next_sol;
// 			}else{
// 				next_sol.destruct();//don't leak the memory
// 			}
// 		}

// 		if (moves < 1){ //count up zero move iterations
// 			zero_moves_count++;//If too many 0's in a row, call the solution good to save time
// 		}else{
// 			zero_moves_count = 0;
// 		}

// 		newFile<<count <<"\t"<< curr_sol.total_HPWL<<"\t"<< curr_sol.get_cost(T) << "\t" << moves<<"\t" << T << endl;
// 		T *= cool_down_factor;
// 	}
// 	printProgress(1.0);
// 	cout << endl;
// 	chip_layout = curr_sol;
// }


// Chip_Layout Chip::perturb(Chip_Layout *curr_sol){
// 	Chip_Layout new_sol = *curr_sol;
// 	//get 2 random grid points to swap
// 	int i1 = random_int(0,(int)new_sol.height-1); //row1
// 	int j1 = random_int(0,new_sol.rows[i1].row.size()-1); //column1
// 	int i2 = random_int(0,(int)new_sol.height-1); //row2
// 	int j2 = random_int(0,new_sol.rows[i2].row.size()-1); //column2

// 	//swap gates
// 	deque<node> temp1, temp2;
// 	iter_swap(new_sol.rows[i1].row.begin() + j1, new_sol.rows[i2].row.begin() + j2);

// 	//clear our layout's two rows
// 	temp1.swap(new_sol.rows[i1].row);
// 	temp2.swap(new_sol.rows[i2].row);
// 	new_sol.rows[i1].curr_width = 0;
// 	new_sol.rows[i2].curr_width = 0;

// 	//redo placement of those rows to update x and y values
// 	for (int i = 0; i < temp1.size(); i++){
// 		new_sol.gate_pushback(temp1[i],i1);
// 	}
// 	for (int i = 0; i < temp2.size(); i++){
// 		new_sol.gate_pushback(temp2[i],i2);
// 	}
// 	// new_sol.calculate_wire_lengths();
// 	new_sol.recalculate_wire_lengths(i1,j1,i2,j2);
// 	return new_sol;
// }


// bool Chip::accept_move(double delta_cost, double temp, double K){
// 	if (delta_cost < 0) { return true; }//we'd be crazy 🤪 not to accept this!!
// 	//if change in cost is not negative, use Boltzman probability to decide to accept
// 	double boltz = exp((-1)*(delta_cost/(K*temp)));
// 	double r = (double)rand()/RAND_MAX; //# between 0 and 1
// 	//accept or reject
// 	if (r < boltz) { 
// 		return true;
// 	}
// 	return false;
// }

// void Chip::destroy_chip(){
// 	chip_layout.width = 0;
// 	chip_layout.height = 0;
// 	// delete [] chip_layout.rows;
// 	chip_layout.rows = nullptr;
// }

/*//////////////////////END SIMULATED ANNEALING////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*			CHIP PRINTOUT FUCNTIONS (TERMINAL AND FILE)							  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void Chip::print(double seconds){
	cout << "-------------------Chip Info----------------------" << endl << endl;
	cout << "Module Count: " << circuit.module_count << endl;
	cout << "Chip width: " << width << " height: " << height << endl;
	cout << "Chip area: " << width*height << endl << endl;

	//print sequence pair (pos_loci, neg_loci)
	cout << "SP: " << endl << "\tPositive Loci = (";
	for (int i = 0; i < pos_loci.size(); i++){
		cout << pos_loci[i];
		if(i < pos_loci.size()-1){
			cout << " ";
		}
	}
	cout << ")" << endl << "\tNegative Loci = (";
	for (int i = 0; i < neg_loci.size(); i++){
		cout << neg_loci[i];
		if(i < neg_loci.size()-1){
			cout << " ";
		}
	}
	cout << ")" << endl << endl;

	//print bottom left cormer of each module
	cout << "Module\tx\ty"<< endl;
	queue<string> m = circuit.module_keys;
	while (!m.empty()){
		string key = m.front();
		m.pop();
		cout << key << "\t" << circuit.modules[key]->x_left << "\t";
		cout << circuit.modules[key]->y_bottom << endl;
	}
	cout << endl << "** Note: x and y are given for the bottom left corner of module **" << endl;	
	cout << endl << "-----------------------------------------------------" << endl;
}

void Chip::write(string in_file, double seconds){
	//create directory for output files
	string output_file;
	mkdir("output", 0777);
	size_t found = in_file.find_last_of("/\\");
	output_file = split(in_file.substr(found+1), "."  )[0];

	ofstream newFile("output/" + output_file + "_Patterson_Christopher.out1");
	if(!newFile.is_open()){
		cout << "Unable to open or create output file: " << "output/" << output_file << "_Patterson_Christopher.out1"<< endl;
		exit(1); 
	}else{
		newFile << "-------------------Chip Info----------------------" << endl << endl;
		newFile << "Module Count: " << circuit.module_count << endl;
		newFile << "Chip width: " << width << " height: " << height << endl;
		newFile << "Chip area: " << width*height << endl << endl;

		//print sequence pair (pos_loci, neg_loci)
		newFile << "Sequence Pairs: " << endl << "\tPositive Loci = (";
		for (int i = 0; i < pos_loci.size(); i++){
			newFile << pos_loci[i] << " ";
		}
		newFile << ")" << endl << "\tNegative Loci = (";
		for (int i = 0; i < neg_loci.size(); i++){
			newFile << neg_loci[i] << " ";
		}
		newFile << ")" << endl << endl;

		//print bottom left cormer of each module
		newFile << "Module\tx\ty"<< endl;
		queue<string> m = circuit.module_keys;
		while (!m.empty()){
			string key = m.front();
			m.pop();
			newFile << key << "\t" << circuit.modules[key]->x_left << "\t";
			newFile << circuit.modules[key]->y_bottom << endl;
		}
		newFile << endl << "** Note: x and y are given for the bottom left corner of module **" << endl;	
		newFile << endl << "-----------------------------------------------------" << endl;	
	}

}

/*//////////////////////END PRINTOUT////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*						Chip Layout Functions									  */
/*						for updating wire lengths.								  */
/*					Current method is to traverse entire chip. 					  */
/*////////////////////////////////////////////////////////////////////////////////*/

// void Chip_Layout::calculate_wire_lengths(){
// 	total_HPWL = 0;
// 	actual_width = 0;
// 	for (int i=0; i<height; i++){
// 		deque<node> row = rows[i].row;
// 		if (rows[i].curr_width > actual_width){
// 			actual_width = rows[i].curr_width;
// 		}
// 		while(!row.empty()){
// 			node gate = row.front();
// 			row.pop_front();
// 			//look at all outputs of this node. 
// 			gate_coordinates[gate.key].wire_length = calculate_wire_length(gate);
// 			total_HPWL += gate_coordinates[gate.key].wire_length ;
// 		}
// 	}
// }

// void Chip_Layout::recalculate_wire_lengths(int gate1_row, int gate1_column, int gate2_row, int gate2_column){
// 	node gate1 = rows[gate1_row].row[gate1_column];
// 	node gate2 = rows[gate2_row].row[gate2_column];

// 	//update the chip's actual width
// 	actual_width = 0;
// 	for (int i=0; i<height; i++){
// 		if (rows[i].curr_width > actual_width){
// 			actual_width = rows[i].curr_width;
// 		}
// 	}

// 	deque<node> gates_to_recalculate;

// 	gates_to_recalculate.push_back(gate1);
// 	gates_to_recalculate.push_back(gate2);
// 	//if the swapped gates are of different length, update all cells that follow them
// 	//in their respective rows
// 	if (gate1.width != gate2.width){
// 		for (int i = gate1_column+1; i <  rows[gate1_row].row.size(); i++){
// 			gates_to_recalculate.push_back(rows[gate1_row].row[i]);
// 		}
// 		for (int i = gate2_column+1; i <  rows[gate2_row].row.size(); i++){
// 			gates_to_recalculate.push_back(rows[gate2_row].row[i]);
// 		}
// 	}
// 	//Next iterate through all output to end of circuit
// 	while(!gates_to_recalculate.empty()){
// 		node temp = gates_to_recalculate.front();

// 		//subtract old gate's HPWL, recalculate HPWL, and add back new HPWL
// 		total_HPWL -= gate_coordinates[temp.key].wire_length;
// 		gate_coordinates[temp.key].wire_length = calculate_wire_length(temp);
// 		total_HPWL += gate_coordinates[temp.key].wire_length;

// 		for (auto elem: temp.inputs){//do the same for this gates inputs
// 			node n = *elem.second;
// 			//subtract old gate's HPWL, recalculate HPWL, and add back new HPWL
// 			total_HPWL -= gate_coordinates[n.key].wire_length;
// 			gate_coordinates[n.key].wire_length = calculate_wire_length(n);
// 			total_HPWL += gate_coordinates[n.key].wire_length;
// 		}
// 		gates_to_recalculate.pop_front();
// 	}
// }

// double Chip_Layout::calculate_wire_length(node n){
// 	double length = 0;
// 	struct Coordinates coord = gate_coordinates[n.key];

// 	//set extreme values so we can compare to our net
// 	double min_x = coord.left_x, min_y = coord.y;
// 	double max_x = coord.left_x, max_y = coord.y;

// 	//iterate through all output gates (net)
// 	for (auto elem: n.outputs) {
// 		double x = gate_coordinates[elem.second->key].left_x;
// 		double y = gate_coordinates[elem.second->key].y;
// 		if (x > max_x){
// 			max_x = x;
// 		}
// 		if (x < min_x){
// 			min_x = x;
// 		}
// 		if (y > max_y){
// 			max_y = y;
// 		}
// 		if (y < min_y){
// 			min_y = y;
// 		} 
// 	}
// 	//we have max and mins, now calculate HPWL
// 	//(2*length of rectangle + 2* height of rectangle)/2
// 	//or length of rectangle + height of rectangle
// 	length = (max_x-min_x)+(max_y-min_y);
// 	return length;
// }

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
		// srand(0); //use this for repeatability
		first = false;
	}
	return min + rand() % (( max + 1 ) - min);
}




// From razzak on https://stackoverflow.com/questions/14539867/how-to-display-a-progress-indicator-in-pure-c-c-cout-printf
void printProgress (double percentage){
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf ("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush (stdout);
}

/*//////////////////////END HELPERS////////////////////////////////////////////*/


