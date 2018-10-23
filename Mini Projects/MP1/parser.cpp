#include <stdio.h>
#include <string>
#include <queue> 
#include <vector> 
#include <map> 
#include <iterator> 
#include <algorithm>
#include <cstring>



using namespace std;

#include "helpers.h" 
#include "parser.h"

int main(int argc, char *argv[]){
    //argc is number of args
    //argv is array of pointers to arguments
	if ((argc == 4) && (strcmp(argv[1],"read_nldm") == 0)){
        //read_nldm
		string fileName = argv[3];
		char *arg = argv[2];
		if (strcmp(arg,"delays") == 0 || strcmp(arg,"slews") == 0){
			lut.assignarrays(fileName);
			writeLUTInfo(arg);
			cout << "NLDM has been parsed. Details can be found in 'in delay_LUT.txt' or 'slew_LUT.txt" << endl;
		}else{
			cout << "Argument: " << arg << " was passed" << endl;
			cout << "'delays' or 'slews' was expected" << endl;
		}
		
    }else if((argc == 3) && (strcmp(argv[1],"read_ckt") == 0)){
        //read_ckt
		string fileName = argv[2];
		createnodes(fileName);
		writeCktInfo();

		cout << "Circuit has been parsed. Details can be found in 'ckt_details.txt'" << endl;

    }else if(argc == 2){
		double circuit_delay, required_time;
		string fileName = argv[1];
		lut.assignarrays("sample_NLDM.lib");
		createnodes(fileName);
		circuit_delay = forwardTraverse(); //get circuit delay and set arrival times
		required_time = circuit_delay * 1.1;
		findSlack(required_time);
		vector<node*> critpath = findCritPath();
		writeSTA(circuit_delay, critpath);
		cout << "Circuit has been parsed and traversed. Details can be found in 'ckt_traversal.txt'" << endl;

	}else{
		cerr << endl;
		cerr << "Usage: " << argv[0] << " read_ckt bench_file" << endl;
		cerr << "		" << "or" << endl;
		cerr << "	" << argv[0] << " read_nldm argument lib_file" << endl;
		cerr << "		" << "or" << endl;
		cerr << "	" << argv[0] << " bench_file" << endl;
		cerr << endl;
		return 1;
	}

    return(0);
}

/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*								TRAVERSALS										  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/
double forwardTraverse(){
	queue <node*> node_queue;
	double circuit_delay = 0.0;
	for (auto thisnode: nodes){
		node_queue.push(thisnode.second);
	}
	while(!node_queue.empty()){
		node* thisnode = node_queue.front();
		if (thisnode->in_degree == 0){//not waiting on anyone
			if (thisnode->is_in){
				thisnode->inp_arrival.push_back(0.0);//node arrival time is 0
				thisnode->Tau_in.push_back(0.002); // slew is 2.0ps
				for (auto output: thisnode->outputs){
					thisnode->Cload += lut.Cap_in[getLUTKey(output.second)]; //Calculate Cload for LUT lookup
				}
				thisnode->outp_arrival.push_back(0.0);
				thisnode->tau_outs.push_back(0.002);
			}else{
				for (auto input: thisnode->inputs){//get input values
					thisnode->inp_arrival.push_back(input.second->max_out_arrival);
					thisnode->Tau_in.push_back(input.second->Tau_out);
				}
				if (thisnode->is_out){
					thisnode->Cload = 4.0*lut.Cap_in["INV_X1"];
				}else{
					for (auto output: thisnode->outputs){
						thisnode->Cload += lut.Cap_in[getLUTKey(thisnode)]; //Calculate Cload for LUT lookup
					}
				}
				thisnode->outp_arrival = calculateOutArrivals(thisnode);
				thisnode->tau_outs = calculateOutTaus(thisnode);
			}
			//We have Cload now and input slew now get arrival times
			double max_out = 0;

			thisnode->max_out_arrival = *max_element(thisnode->outp_arrival.begin(),thisnode->outp_arrival.end());
			thisnode->Tau_out = *max_element(thisnode->tau_outs.begin(),thisnode->tau_outs.end());
			if (thisnode->is_out && thisnode->max_out_arrival*1000 > circuit_delay){
				// cout << thisnode->name << " : " <<  thisnode->max_out_arrival*1000 << endl;
				circuit_delay = thisnode->max_out_arrival*1000;
			}
			for(auto out: thisnode->outputs){//update input outdegrees
				out.second->in_degree--;
			}
			node_queue.pop();
		}else{//not ready for this node yet
			node_queue.pop();
			node_queue.push(thisnode);
		}
	}
	return circuit_delay;
}

void findSlack(double required_time){
	queue <node*> node_queue;
	for (auto thisnode: nodes){
		node_queue.push(thisnode.second);
	}
	while(!node_queue.empty()){
		double required = required_time;
		node* thisnode = node_queue.front();
		node_queue.pop();
		if (thisnode->out_degree == 0){//not waiting on anyone
			if (!thisnode->is_out){
				//This nodes required arrival is the min of all output required times
				for(auto out: thisnode->outputs){//get earliest required time of outputs
					double temp =  out.second->required_input_time - out.second->cell_delays[thisnode->name];
					if (temp <= required){
						required = temp;
					}
				}
			}else{
				required = required_time;
			}
			thisnode->slack = required - thisnode->max_out_arrival*1000;
			thisnode->required_input_time = required;
			for(auto in: thisnode->inputs){//update input outdegrees
				in.second->out_degree--;
			}
		}else{//not ready for this node yet
			node_queue.push(thisnode);
		}
	}
}

vector<node*> findCritPath(){
	vector<node*> path;
	node* current_node;
	double current_min = numeric_limits<double>::infinity();
	for (auto thisnode: nodes){
		if(thisnode.second->is_out && thisnode.second->slack < current_min){
			current_min = thisnode.second->slack;
			current_node = thisnode.second;
		}
	} 
	path.push_back(current_node);
	while(!current_node->is_in){
		current_min = numeric_limits<double>::infinity();
		for(auto in: current_node->inputs){
			if(in.second->slack < current_min){
				current_min = in.second->slack;
				current_node = in.second;
			}
		}
		path.push_back(current_node);
	}
	return path;
}

vector<double> calculateOutArrivals(node* thisnode){//calculate outp_arrival vector
	vector<double>  out;
	double max_arrival = 0;
	double num_inputs = 1;
	if (!thisnode->is_in){
		num_inputs = double(thisnode->inputs.size());
	}
	double multiplier = 1.0;//Will be 1 for 1-inp gates like inverters and buffs
	if (num_inputs > 1.0){//If its a 2 or more input gate, multiply Tau by n/2
		double multiplier = num_inputs/2;
	}
	int i = 0;
	for (auto input: thisnode->inputs){ 
		double cellDelay = multiplier*getCell(thisnode, input.second->Tau_out, 0);
		double arrival = thisnode->inp_arrival[i] + cellDelay;
		thisnode->cell_delays[input.second->name] = cellDelay*1000;
		out.push_back(thisnode->inp_arrival[i] + cellDelay);
		i++;
	}
	return out;
} 

vector<double> calculateOutTaus(node* thisnode){//calculate out Taus vector
	vector<double> out;
	double num_inputs = 2;
	if (!thisnode->is_in){
		num_inputs = double(thisnode->inputs.size());
	}
	double multiplier = 1.0;//Will be 1 for 1-inp gates like inverters and buffs
	if (num_inputs > 1.0){//If its a 2 or more input gate, multiply Tau by n/2
		double multiplier = num_inputs/2;
	}
	for (auto input: thisnode->inputs){ 
		double Tau = multiplier*getCell(thisnode, input.second->Tau_out, 1);
		out.push_back(Tau);
	}
	return out;
} 

double getCell(node* thisnode,double tau_in, int type){//type: 0=delay, 1=slew
	vector<vector<double>> table;
	double cload = thisnode->Cload;
	double t_1, t_2, c_1, c_2,v11,v12,v21,v22;
	double out;
	int j,k;
	string key = getLUTKey(thisnode);

	if (type ==0){
		table = lut.All_delays[key];
	}else{
		table = lut.All_slews[key];
	}
	vector<double> TauVals = lut.Tau_in_vals[key];
	vector<double> CloadVals = lut.Cload_vals[key];

	//get table indicies
	if (tau_in > TauVals[TauVals.size()-1]){
		j = TauVals.size()-1;
	}else if(tau_in < TauVals[0]){
		j = 1;
	}else{
		for(j = 0; j < TauVals.size(); j++){
			//τ1 ≤ τ < τ2 
			if (TauVals[j] > tau_in && TauVals[j-1] <= tau_in){
				break;
			}
		}
	}
	if (cload > CloadVals[CloadVals.size()-1]){
		k = CloadVals.size()-1;
	}else if(cload < CloadVals[0]){
		k = 1;
	}else{
		for(k = 0; k < CloadVals.size(); k++){
			//  C1 ≤ C < C2
			if (CloadVals[k] > cload && CloadVals[k-1] <= cload){
				break;
			}
		}	
	}

	t_1 = TauVals[j-1];
	t_2 = TauVals[j];
	c_1 = CloadVals[k-1];
	c_2 = CloadVals[k];
	v11 = table[j-1][k-1];
	v12 = table[j-1][k];
	v21 = table[j][k-1];
	v22 = table[j][k];

	//input slew as first index, output load as 2nd index
	out = v11*(c_2-cload)*(t_2-tau_in);
	out += v12*(cload-c_1)*(t_2-tau_in);
	out += v21*(c_2-cload)*(tau_in-t_1);
	out += v22*(cload-c_1)*(tau_in-t_1);
	out = out/((c_2-c_1)*(t_2-t_1));
	return out;
}

string getLUTKey(node* thisnode){//TODO: CHECK WITH PROF
	if(thisnode->inp_arrival.size() > 1){//If more then one input, assume not INV or BUF
		return thisnode->type + "2_X1";
	}else{
		if (stringContains(thisnode->type,"BUFF")){
			return "BUF_X1";
		}else if (stringContains(thisnode->type,"NOT")){
			return "INV_X1";
		}else {
			return thisnode->type + "_X1";
		}
	}
}



/*//////////////////////END TRAVERSALS ///////////////////////////////////////////*/




/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*								DATA STRUCTURE INIT								  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void createnodes(string bench_file){
	string fileLine;
	ifstream inFile;

    inFile.open(bench_file);
    if (!inFile) {
        cout << "Unable to open input file" << endl;
        exit(1); 
    }

	while (getline(inFile, fileLine)){
		if(stringContains(fileLine,"INPUT")){
			//input node
			readLine_ckt(fileLine,1);
		}else if(stringContains(fileLine,"OUTPUT")){
			//output node
			readLine_ckt(fileLine,2); 
		}else if (stringContains(fileLine,"=")){
			//inner node
			readLine_ckt(fileLine,0); 
		}
	} 

    inFile.close();
}

//define the arrays to be used during STA call later.
//also helps to simply assign the arrays so that a call to this function will fetch the arrays, and you can easily print out the details of this NLDM
void LUT::assignarrays(string NLDM_file){

	string fileLine;
	ifstream inFile;
    
    inFile.open(NLDM_file);
    if (!inFile) {
        cout << "Unable to open file" << endl;
        exit(1);
    }

	string cellName, valueType; //This will update everytime a new cell is being parsed
	//Keeping track of if we are on delays or slews
	while (getline(inFile, fileLine)){
		if(stringContains(fileLine,"cell ")){
			cellName = split(split(fileLine,"(")[1],")")[0];
			this->Allgate_name.push_back(cellName);
		}else if (stringContains(fileLine,"capacitance		:")){
			this->Cap_in[cellName] = stod(split(split(fileLine,": ")[1],";")[0]);
		}else if (stringContains(fileLine,"index_1 ")){
			this->Tau_in_vals[cellName] = vectorStr2Dbl(split(split(fileLine,"\"")[1],","));
		}else if (stringContains(fileLine,"index_2 ")){
			this->Cload_vals[cellName] = vectorStr2Dbl(split(split(fileLine,"\"")[1],","));
		}else if (stringContains(fileLine,"cell_delay")){
			valueType = "delays";
		}else if (stringContains(fileLine,"output_slew")){
			valueType = "slews";
		}else if (stringContains(fileLine,"values ")){ //get multiple lines to fill all values
			// parse values
			int i = 0;
			while(stringContains(fileLine,"\"")){//While reading in values
				if (valueType == "delays"){
					this->All_delays[cellName].push_back(vectorStr2Dbl(split(split(fileLine,"\"")[1],",")));
				}else{
					this->All_slews[cellName].push_back(vectorStr2Dbl(split(split(fileLine,"\"")[1],",")));					
				}
				i++;
				getline(inFile, fileLine);
			}
		} 
	}
    inFile.close();
}

//i: 1=input, 2=output,default=other
void readLine_ckt(string fileLine,int i){
	unsigned first, last;
	string key, type, name;
	first = fileLine.find("(");
	last = fileLine.find(")");
	switch (i)
	{
		case 1: //INPUT NODE
			key = fileLine.substr(first+1,last-first-1); //get n#
			nodes[key] = new node("INP-"+key, "INP", false, true); 
			nodes_queue.push(nodes[key]);
			break;
		case 2:
			key = fileLine.substr(first+1,last-first-1); //get n#
			//update node or create one if does not exist
			if ( nodes.find(key) == nodes.end() ) {
				//node not found. New node to map
				nodes[key] = new node("NULL-"+key, "OUTP", true, false); 
			}else{
				nodes[key]->name = name;
				nodes[key]->type = type;
				nodes[key]->is_out = true;
			}
			nodes_queue.push(nodes[key]);
			break;
		default:
			vector <string> fields,gate,fanInKeys;
			string unparsedInNodes;
			//parse into node attributes
			fields = split(fileLine, " = "  ); //[outputkey, GATE(input,keys)]
			gate = split(fields[1], "("  ); //[GATE, input,keys)]
			unparsedInNodes = split(gate[1], ")"  )[0];//"input, keys, delimited, by, comma and space"
			fanInKeys = split(unparsedInNodes, ", "); //[input, nodes, as , vector]

			//set values to something we can remember
			key = fields[0];
			type = gate[0];
			name = type+'-'+key;

			//update node or create one if does not exist
			if ( nodes.find(key) == nodes.end() ) {
				//node not found. New node to map
				nodes[key] = new node(name, type, false, false); 
			}else{
				nodes[key]->name = name;
				nodes[key]->type = type;
			}
			//get fanin and set fanout of nodesMap
			for(string inkey : fanInKeys){
				nodes[key]->inputs[inkey] = nodes[inkey];
				nodes[key]->in_degree++;
				
				nodes[inkey]->outputs[key] = nodes[key];
				nodes[inkey]->out_degree++;
			} 
			nodes_queue.push(nodes[key]);
	}
}

/*//////////////////////END DATA STRUCTURE INIT//////////////////////////////////////*/





/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*						PRINT DATA TO OUTPUT FILES								  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void writeLUTInfo(char *arg){
	string file;
	if(strcmp(arg,"delays")==0){
		file = "delay_LUT.txt";
	}else{
		file = "slew_LUT.txt";
	}
	ofstream newFile(file);

	newFile << "-----------------------------------------------------" << endl;

	for(auto gateName: lut.Allgate_name){
		newFile << "cell: " << gateName << endl;
		newFile << "input slews: ";
		for(auto value = lut.Tau_in_vals[gateName].begin(); value != lut.Tau_in_vals[gateName].end(); ++value){
			newFile << *value;
			if(next(value) != lut.Tau_in_vals[gateName].end()){
				newFile << ",";
			}
		}
		newFile << endl << "load cap: ";
		for(auto value = lut.Cload_vals[gateName].begin(); value != lut.Cload_vals[gateName].end(); ++value){
			newFile << *value;
			if(next(value) != lut.Cload_vals[gateName].end()){
				newFile << ",";
			}
		}
		newFile << endl << endl;
		if(strcmp(arg,"delays")==0){
			newFile << "delays:" << endl;
			for(auto row: lut.All_delays[gateName]){
				for(auto value = row.begin(); value != row.end(); ++value){
					newFile << *value;
					if(next(value) != row.end()){
						newFile << ",";
					}
				}
				newFile << ";" << endl;
			}
		}else{
			newFile << "slews:" << endl;
			for(auto row: lut.All_slews[gateName]){
				for(auto value = row.begin(); value != row.end(); ++value){
					newFile << *value;
					if(next(value) != row.end()){
						newFile << ",";
					}
				}
				newFile << ";" << endl;
			}	
		}
		newFile << endl << endl << endl;
	}
	newFile << "-----------------------------------------------------" << endl;
}

void writeCktInfo(){
	map <string, int> counts;

	int inCount = 0, outCount = 0;

	ofstream newFile("ckt_details.txt");
	if(!newFile.is_open()){
		cout << "Unable to open or create output file" << endl;
		exit(1); 
	}

	newFile << "-----------------------------------------------------" << endl;
	//Print details to file
	for (auto node: nodes) {
		if (node.second->is_in){
			inCount++;
		}else{
			if ( counts.find(node.second->type) == counts.end() ) {
				//gate not found. Add new gate count
				counts[node.second->type] = 1; 
			}else{
				counts[node.second->type]++;
			}
		}
		if (node.second->is_out){ //can be output and another gate
			outCount++;
		}
	} 

	//print counts
	newFile << inCount << " primary inputs" << endl;
	newFile << outCount << " primary outputs" << endl;
	for (auto count: counts) {
		newFile << count.second << " " << count.first << " gates" << endl;
	} 

	//print fanin and fanout
	newFile << endl << endl;
	newFile << "Fanout..." << endl;
	for (auto node: nodes) {
		if (node.second->is_out){
			newFile << node.second->name << ": OUTP" << endl;
		}else if (!node.second->is_in){
			newFile << node.second->name << ": ";
			const auto& lastKey = node.second->outputs.rbegin()->first;
			for (auto out: node.second->outputs) {
				newFile << out.second->name;
				if(out.first != lastKey){
					newFile << ",";
				}
				newFile << " ";
			}
			newFile << endl;
		}
	}
	newFile << endl << endl;
	newFile << "Fanin..." << endl;
	for (auto node: nodes) {
		if (!node.second->is_in){
			newFile << node.second->name << ": ";
			const auto& lastKey = node.second->inputs.rbegin()->first;
			for (auto out: node.second->inputs) {
				newFile << out.second->name;
				if(out.first != lastKey){
					newFile << ",";
				}
				newFile << " ";
			}
			newFile << endl;
		}
	}
	newFile << "-----------------------------------------------------" << endl;
}

void writeSTA(double circuit_delay, vector<node*> critpath){
	//Print it all out
	ofstream newFile("ckt_traversal.txt");
	if(!newFile.is_open()){
		newFile << "Unable to open or create output file" << endl;
		exit(1); 
	}
	newFile << "-----------------------------------------------------" << endl;

	newFile << "Circuit delay: " << circuit_delay << " ps" << endl << endl;

	newFile << "Gate slacks: " << endl;

	// for (auto node: nodes){
	// 	newFile << node.second->name << ": " << node.second->slack << " ps" << endl;
	// }
	while(!nodes_queue.empty()){
		node* node = nodes_queue.front();
		nodes_queue.pop();
		if (node->is_out){
			newFile << "OUTPUT-" << split(node->name,"-")[1] <<": ";
			node->is_out = false; //don't need this after, set to false so we can print the gate later too
		}else{
			newFile << node->name << ": ";
		}
		newFile << node->slack << " ps" << endl;

	}

	newFile << endl << "Critical path: " << endl;

	for (int i = critpath.size()-1; i >=0 ; i--){
		if(i>0){
			newFile << critpath[i]->name << ","; 
		}else{
			newFile << critpath[i]->name << "," << "OUTPUT-" << split(critpath[i]->name,"-")[1] << endl; 
		}
	} 

	newFile << endl <<"-----------------------------------------------------" << endl;
}

/*//////////////////////END PRINT OUT////////////////////////////////////////////*/






