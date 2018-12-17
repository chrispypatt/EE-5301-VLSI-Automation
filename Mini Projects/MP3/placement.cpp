#include "parser.h"
#include "helpers.h"
#include "graph.h"
#include "placement.h"
#include <limits>
#include <math.h>

int main(int argc, char *argv[]){
	Circuit circuit;
	Chip chip;
	clock_t start, end;

	mkdir("output", 0777);

	
    // argc is number of args
    // argv is array of pointers to arguments
	if(argc == 2){
		cout << "-----------------------------------------------------" << endl;
		string file_name = argv[1];
		circuit.parseNetlist(file_name);

		//initialize netlist placement on chip
		chip = Chip(circuit);

		chip.write(file_name,-1.0, "");
		cout << "-----------------------------------------------------" << endl;
    }else if (argc == 3){
		string file_name = argv[1];
		string option = argv[2];
		if (stringContains(option,"-a") || stringContains(option,"-w") || stringContains(option,"-c")){
			cout << "-----------------------------------------------------" << endl;
			circuit.parseNetlist(file_name);

			string sig;
			if (option == "-a"){
				sig = "a";
			}else if (option == "-w"){
				sig = "w";
			}else{
				sig = "c";
			}

			//timer for initial placing and annealing
			start = clock();

			chip = Chip(circuit);
			chip.simulated_annealing(file_name, sig);

			end = clock(); 
			auto duration = ((float)end-(float)start)/CLOCKS_PER_SEC; 

			chip.write(file_name, duration, sig);
			cout << "-----------------------------------------------------" << endl;
		}else{
			cerr << endl;
			cerr << "Usage: " << argv[0] << " <bench_file>" << endl;
			cerr << "		" << "or" << endl;
			cerr << "	" << argv[0] << " <bench_file> <option={-a,-w,-c}>" << endl;
			cerr << endl;
			return 1;
		}
	}else{
		cerr << endl;
		cerr << "Usage: " << argv[0] << " <bench_file>" << endl;
		cerr << "		" << "or" << endl;
		cerr << "	" << argv[0] << " <bench_file> <option={-a,-w,-c}>" << endl;
		cerr << endl;
		return 1;
	}

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

	default_random_engine rng(time(NULL)); 
	// default_random_engine rng(0);//use this for reapeatability
	shuffle(begin(module_rp), std::end(module_rp), rng);
	shuffle(begin(module_rn), std::end(module_rn), rng);

	//initalize data structures
	curr_placement.Gh = Graph(ckt.module_keys.size());
	curr_placement.Gv = Graph(ckt.module_keys.size());
	int index = 0;
	while(!module_rp.empty()){
		string key_p = module_rp.back();
		string key_n = module_rn.back();

		//set up positive and negative loci
		curr_placement.pos_loci.push_back(key_p);
		curr_placement.neg_loci.push_back(key_n);
		curr_placement.pos_index[key_p] = index;
		curr_placement.neg_index[key_n] = index;
		curr_placement.Gv.has_incoming[key_p] = false;
		curr_placement.Gh.has_incoming[key_p] = false;

		index++;
		module_rp.pop_back();
		module_rn.pop_back();
	} 

	curr_placement.find_placement(circuit);
	curr_placement.calculate_wire_lengths(ckt.edges);
	s_area = curr_placement.width*curr_placement.height;
	s_hpwl = curr_placement.HPWL;

	cout << "Initial placement completed!" << endl << endl;
}

/*//////////////////////END Chip Layout////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*					CHIP SIMULATED ANNEALING									  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void Chip::annealing_init(string option){
	double prev_cost, next_cost;
	double avg_cost = 0;
	Placement curr, next;
	curr = curr_placement;
	for (int i = 0; i < 50; i++){
		prev_cost = curr.get_cost(option);
		next = perturb(&curr);
		next.find_placement(circuit);
		next.calculate_wire_lengths(circuit.edges);
		next_cost = next.get_cost(option);
		avg_cost += abs(next_cost-prev_cost);
	}
	avg_cost /= 50;
	K = abs(avg_cost/(log(.9)*T_O));
}

void Chip::simulated_annealing(string in_file, string option){
	string output_file;
	size_t found = in_file.find_last_of("/\\");
	output_file = split(in_file.substr(found+1), "."  )[0];
	
	ofstream newFile("output/intermediate_details_" + output_file +  ".txt"+option);

	//Initital solution is the initialized llayout
	double delta_cost;
	int count = 0, zero_moves_count = 0;

	double cool_down_factor = 0.95;//T_O*.01;
	double T = T_O;

	annealing_init(option);

	cout << "Conducting Simulated Annealing" << "." << "." << "." << endl;
	newFile <<"Iteration" << "," <<"Cost" <<","  <<"HPWL"<< ","<<"Area," << "Moves" <<",T,"<< endl;
	newFile <<"START" << ","<< curr_placement.get_cost(option)<<","<< curr_placement.HPWL << ","<< curr_placement.height*curr_placement.width << "," << "--"<<"," << T <<"," << endl;
	
	//perform annealing!!
	while (T > T_FREEZE && zero_moves_count < 10){//allow for faster termination if consectively stagnent
		count++;
		int moves = 0;
		// printProgress((double)count/167.0); //peace of mind to user. not always accurate
		for (int i = 0; i < NUM_MOVES_PER_STEP; i++){
			//perturb and recalculate HPWL
			string first="", second="";
			Placement new_placement = perturb(&curr_placement);

			new_placement.find_placement(circuit);


			new_placement.calculate_wire_lengths(circuit.edges);

			delta_cost = new_placement.get_cost(option) - curr_placement.get_cost(option);

			if (accept_move(delta_cost, T, K)){
				moves++;
				curr_placement.destruct();//don't leak the memory
				curr_placement = new_placement;
			}else{
				new_placement.destruct();//don't leak the memory
			}

		}

		if (moves < 1){ //count up zero move iterations
			zero_moves_count++;//If too many 0's in a row, call the solution good to save time
		}else{
			zero_moves_count = 0;
		}

		newFile << count <<","<< curr_placement.get_cost(option)<<","<< curr_placement.HPWL << ","<< curr_placement.height*curr_placement.width << "," << moves<<"," << T << "," << endl;
		T *= cool_down_factor;
	}
	cout << "Simulated Annealing Completed!" << endl;
	// printProgress(1.0);
	cout << endl;
}


Placement Chip::perturb(Placement *curr_sol){
	Placement new_sol = *curr_sol;

	//invalidate old data
	new_sol.HPWL = -1;
	new_sol.height = -1;
	new_sol.width = -1;

	//get 2 modules from each loci to swap
	int i1 = random_int(0,circuit.module_count-1); 
	int j1 = random_int(0,circuit.module_count-1); 
	int i2 = random_int(0,circuit.module_count-1); 
	int j2 = random_int(0,circuit.module_count-1); 

	//swap loci
	new_sol.pos_index[new_sol.pos_loci[i1]] = i2;
	new_sol.pos_index[new_sol.pos_loci[i2]] = i1;
	new_sol.neg_index[new_sol.neg_loci[j1]] = j2;
	new_sol.neg_index[new_sol.neg_loci[j2]] = j1;
	
	string temp = new_sol.pos_loci[i1];
	new_sol.pos_loci[i1] = new_sol.pos_loci[i2];
	new_sol.pos_loci[i2] = temp;

	temp = new_sol.neg_loci[j1];
	new_sol.neg_loci[j1] = new_sol.neg_loci[j2];
	new_sol.neg_loci[j2] = temp;
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

/*//////////////////////END SIMULATED ANNEALING////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*			CHIP PRINTOUT FUCNTIONS (TERMINAL AND FILE)							  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void Chip::print(double seconds){
	if (seconds < 0){//output for phase 1
		cout << "-------------------Chip Info----------------------" << endl << endl;
		cout << "Module Count: " << circuit.module_count << endl;
		cout << "Chip width: " << curr_placement.width << " height: " << curr_placement.height << endl;
		cout << "Chip area: " << curr_placement.width*curr_placement.height << endl << endl;

		//print sequence pair (pos_loci, neg_loci)
		cout << "SP: " << endl << "\tPositive Loci = (";
		for (int i = 0; i < curr_placement.pos_loci.size(); i++){
			cout << curr_placement.pos_loci[i];
			if(i < curr_placement.pos_loci.size()-1){
				cout << " ";
			}
		}
		cout << ")" << endl << "\tNegative Loci = (";
		for (int i = 0; i < curr_placement.neg_loci.size(); i++){
			cout << curr_placement.neg_loci[i];
			if(i < curr_placement.neg_loci.size()-1){
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
			cout << key << "\t" << curr_placement.module_coordinates[key].x << "\t";
			cout << curr_placement.module_coordinates[key].y << endl;
		}
		cout << endl << "** Note: x and y are given for the bottom left corner of module **" << endl;	
		cout << endl << "-----------------------------------------------------" << endl;
	}else{//output for phase 2
		cout << "-------------------Chip Info----------------------" << endl << endl;
		cout << "Execution Time: " << seconds << "s" << endl;
		cout << "Chip width: " << curr_placement.width << " height: " << curr_placement.height << endl;
		cout << "Chip area: " << curr_placement.width*curr_placement.height << " (initially " << s_area << ")"<< endl;
		cout << "Total HPWL: " << curr_placement.HPWL  << " (initially " << s_hpwl << ")"<< endl << endl;

		//print sequence pair (pos_loci, neg_loci)
		cout << "SP: " << endl << "\tPositive Loci = (";
		for (int i = 0; i < curr_placement.pos_loci.size(); i++){
			cout << curr_placement.pos_loci[i];
			if(i < curr_placement.pos_loci.size()-1){
				cout << " ";
			}
		}
		cout << ")" << endl << "\tNegative Loci = (";
		for (int i = 0; i < curr_placement.neg_loci.size(); i++){
			cout << curr_placement.neg_loci[i];
			if(i < curr_placement.neg_loci.size()-1){
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
			cout << key << "\t" << curr_placement.module_coordinates[key].x << "\t";
			cout << curr_placement.module_coordinates[key].y << endl;
		}
		cout << endl << "** Note: x and y are given for the bottom left corner of module **" << endl;	
		cout << endl << "-----------------------------------------------------" << endl;
	}
}

void Chip::write(string in_file, double seconds, string option){
	//create directory for output files
	string output_file;
	size_t found = in_file.find_last_of("/\\");
	output_file = split(in_file.substr(found+1), "."  )[0];

	if (seconds < 0){//output for phase 1
		ofstream newFile("output/" + output_file + "_Patterson_Christopher.out1");
		if(!newFile.is_open()){
			cout << "Unable to open or create output file: " << "output/" << output_file << "_Patterson_Christopher.out1"<< endl;
			exit(1); 
		}else{
			newFile << "-------------------Chip Info----------------------" << endl << endl;
			newFile << "Module Count: " << circuit.module_count << endl;
			newFile << "Chip width: " << curr_placement.width << " height: " << curr_placement.height << endl;
			newFile << "Chip area: " << curr_placement.width*curr_placement.height << endl << endl;

			//print sequence pair (pos_loci, neg_loci)
			newFile << "Sequence Pairs: " << endl << "\tPositive Loci = (";
			for (int i = 0; i < curr_placement.pos_loci.size(); i++){
				newFile << curr_placement.pos_loci[i] << " ";
			}
			newFile << ")" << endl << "\tNegative Loci = (";
			for (int i = 0; i < curr_placement.neg_loci.size(); i++){
				newFile << curr_placement.neg_loci[i] << " ";
			}
			newFile << ")" << endl << endl;

			//print bottom left cormer of each module
			newFile << "Module\tx\ty"<< endl;
			queue<string> m = circuit.module_keys;
			while (!m.empty()){
				string key = m.front();
				m.pop();
				newFile << key << "\t" << curr_placement.module_coordinates[key].x << "\t";
				newFile << curr_placement.module_coordinates[key].y << endl;
			}
			newFile << endl << "** Note: x and y are given for the bottom left corner of module **" << endl;	
			newFile << endl << "-----------------------------------------------------" << endl;	
		}
	}else{//output for phase 2
		ofstream newFile("output/" + output_file + "_Patterson_Christopher.out2" + option);
		newFile << "-------------------Chip Info----------------------" << endl << endl;
		newFile << "Execution Time: " << seconds << "s" << endl;
		newFile << "Chip width: " << curr_placement.width << " height: " << curr_placement.height << endl;
		newFile << "Chip area: " << curr_placement.width*curr_placement.height << " (initially " << s_area << ")"<< endl;
		newFile << "Total HPWL: " << curr_placement.HPWL  << " (initially " << s_hpwl << ")"<< endl << endl;

		//print sequence pair (pos_loci, neg_loci)
		newFile << "SP: " << endl << "\tPositive Loci = (";
		for (int i = 0; i < curr_placement.pos_loci.size(); i++){
			newFile << curr_placement.pos_loci[i];
			if(i < curr_placement.pos_loci.size()-1){
				newFile << " ";
			}
		}
		newFile << ")" << endl << "\tNegative Loci = (";
		for (int i = 0; i < curr_placement.neg_loci.size(); i++){
			newFile << curr_placement.neg_loci[i];
			if(i < curr_placement.neg_loci.size()-1){
				newFile << " ";
			}
		}
		newFile << ")" << endl << endl;

		//print bottom left cormer of each module
		newFile << "Module\tx\ty"<< endl;
		queue<string> m = circuit.module_keys;
		while (!m.empty()){
			string key = m.front();
			m.pop();
			newFile << key << "\t" << curr_placement.module_coordinates[key].x << "\t";
			newFile << curr_placement.module_coordinates[key].y << endl;
		}
		newFile << endl << "** Note: x and y are given for the bottom left corner of module **" << endl;	
		newFile << endl << "-----------------------------------------------------" << endl;
	}
}

/*//////////////////////END PRINTOUT////////////////////////////////////////////*/


/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*						Placement Functions										  */
/*						for updating wire lengths.								  */
/*					Current method is to traverse entire chip. 					  */
/*////////////////////////////////////////////////////////////////////////////////*/




void Placement::find_placement(Circuit circuit){
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

	// add edge with weight 0 from s to all nodes without an incoming edge
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
			module_coordinates[elem.first].y = y;
			if (height < y + circuit.modules[elem.first]->height){ //find furthest top edge
				height = y + circuit.modules[elem.first]->height;
			}
		}
	}
	for (auto elem: x_positions){
		if (elem.first != "S"){
			//left edge calculation
			double x = elem.second;
			module_coordinates[elem.first].x = x;
			if (width < x + circuit.modules[elem.first]->width){ //find furthest right edge
				width = x+ circuit.modules[elem.first]->width;
			}
		}
	}
}

double Placement::get_cost(string option){
	double alpha;
	if (option == "a"){
		alpha = 1.0;
	}else if (option == "w"){
		alpha = 0.0;
	}else{
		alpha = 0.7;
	}
	return  alpha*width*height + (1-alpha)*HPWL;	
}

void Placement::calculate_wire_lengths(vector<HyperEdge> edges){
	HPWL = 0;
	for (int i = 0; i < edges.size(); i++){
		double min_x = std::numeric_limits<double>::max(), min_y = std::numeric_limits<double>::max();
		double max_x = 0, max_y = 0;
		for (int j = 0; j < edges[i].degree; j++){
			double x = module_coordinates[edges[i].edge_modules[j]].x;
			double y = module_coordinates[edges[i].edge_modules[j]].y;
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
		HPWL += (max_x-min_x)+(max_y-min_y);
	}
}

/*//////////////////////END Placement ////////////////////////////////////////////*/


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


