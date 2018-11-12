#include <stdio.h>
#include <string>
#include <queue> 
#include <map> 

using namespace std;

#include "helpers.h" 
#include "parser.h"

		
void Circuit::parseNetlist(string file_name){
	string file_line;
	ifstream inFile;

	//Init any values we need to
	total_gates_area = 0;  

    inFile.open(file_name);
    if (!inFile) {
        cout << "Unable to open input file" << endl;
        exit(1); 
    }

	while (getline(inFile, file_line)){
		if(stringContains(file_line,"INPUT")){
			//input node
			createNode(file_line,1);
		}else if(stringContains(file_line,"OUTPUT")){
			//output node
			createNode(file_line,2); 
		}else if (stringContains(file_line,"=")){
			//inner node
			createNode(file_line,0); 
		}
	} 
	setGateWidths();
    inFile.close();
}



/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*								DATA STRUCTURE INIT								  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

//i: 1=input, 2=output,default=other
void Circuit::createNode(string file_line, int i){
	unsigned first, last;
	string key, type, name;
	node *temp;
	first = file_line.find("(");
	last = file_line.find(")");
	switch (i)
	{
		case 1: //INPUT NODE
			key = "INP-" + file_line.substr(first+1,last-first-1); //get n#
			temp = new node(key,key, "INP", false, true); 
			addNode(key,temp);
			break;
		case 2:
			key = "OUTP-" + file_line.substr(first+1,last-first-1); //get n#
			temp = new node(key,key, "OUTP", true, false); 
			addNode(key,temp);
			break;
		default:
			vector <string> fields, gate, fan_in_keys;

			//parse into node attributes
			fields = split(file_line, " = "  ); //[outputkey, GATE(input,keys)]
			gate = split(fields[1], "("  ); //[GATE, input,keys)]
			fan_in_keys = split(split(gate[1], ")"  )[0], ", "); //[input, nodes, as , vector]

			//set values to something we can remember
			type = gate[0];
			key = fields[0];
			name = type+'-'+fields[0];
			temp = new node(name,key, type, false, false); 

			addNode(key,temp);
			setFanInOut(key,fan_in_keys);
			break;
	}
}

void Circuit::setFanInOut(string key, vector <string> fan_in_keys){
	//get fanin and set fanout of nodesMap
	for(string inkey : fan_in_keys){
		if(nodes.find("INP-" + inkey) != nodes.end()){
			inkey = "INP-" + inkey;
		}
		nodes[key]->inputs[inkey] = nodes[inkey];
		nodes[key]->in_degree++;
		
		nodes[inkey]->outputs[key] = nodes[key];
		nodes[inkey]->out_degree++;
	} 
	//If connected to output
	string out_key = "OUTP-" + key;
	if (nodes.find(out_key) != nodes.end()){
		nodes[out_key]->inputs[key] = nodes[key];
		nodes[out_key]->in_degree++;

		nodes[key]->outputs[out_key] = nodes[out_key];
		nodes[key]->out_degree++;
	}
}

void Circuit::setGateWidths(){
	for (auto elem: nodes) {
		node *n = elem.second;
		switch (gate_id[n->type])
		{
			case 0: //NAND
				n->setDimensions(((double)n->in_degree/2.0)*2.0, 1.0);
				break;
			case 1: //NOR
				n->setDimensions(((double)n->in_degree/2.0)*3.0, 1.0);
				break;
			case 2: //XOR
				n->setDimensions(((double)n->in_degree/2.0)*5.0, 1.0);
				break;
			case 3: //AND
				n->setDimensions(((double)n->in_degree/2.0)*3.0, 1.0);
				break;
			case 4: //OR
				n->setDimensions(((double)n->in_degree/2.0)*4.0, 1.0);
				break;
			case 5: //XNOR
				n->setDimensions(((double)n->in_degree/2.0)*6.0, 1.0);
				break;
			case 6: //BUFF
				n->setDimensions(2.0, 1.0);
				break;
			default: //NOT, INP and OUTP
				n->setDimensions(1.0, 1.0);
				break;
		}
		total_gates_area += n->area;
	} 
}


double node::calculate_wire_length(){
	double length = 0;
	//set extreme values so we can compare to our net
	double min_x = left_x, min_y = y;
	double max_x = left_x, max_y = y;

	//iterate through all output gates (net)
	for (auto elem: outputs) {
		double x = elem.second->left_x;
		double y = elem.second->y;
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
	// cout << length << endl;
	return length;
}


/*//////////////////////END DATA STRUCTURE INIT//////////////////////////////////////*/





/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*						PRINT DATA TO OUTPUT FILES								  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

void Circuit::writeCktInfo(){
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
		}else if (node.second->is_out){ //can be output and another gate
			outCount++;
		}else{
			if ( counts.find(node.second->type) == counts.end() ) {
				//gate not found. Add new gate count
				counts[node.second->type] = 1; 
			}else{
				counts[node.second->type]++;
			}
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
	queue <node*> temp_queue = nodes_queue; //copy the original queue to the temporary queue
	while (!temp_queue.empty()){
		node *n = temp_queue.front();
		if (!(n->is_in || n->is_out)){
			newFile << n->name << ": ";
			const auto& lastKey = n->outputs.rbegin()->first;
			for (auto out: n->outputs) {
				newFile << out.second->name;
				if(out.first != lastKey){
					newFile << ",";
				}
				newFile << " ";
			}
			newFile << endl;
		}
		temp_queue.pop();
	} 

	newFile << endl << endl;
	newFile << "Fanin..." << endl;
	temp_queue = nodes_queue; //copy the original queue to the temporary queue
	while (!temp_queue.empty()){
		node *n = temp_queue.front();
		if (!(n->is_in || n->is_out)){
			newFile << n->name << ": ";
			const auto& lastKey = n->inputs.rbegin()->first;
			for (auto out: n->inputs) {
				newFile << out.second->name;
				if(out.first != lastKey){
					newFile << ",";
				}
				newFile << " ";
			}
			newFile << endl;
		}
		temp_queue.pop();
	}
	newFile << "-----------------------------------------------------" << endl;
}

/*//////////////////////END PRINT OUT////////////////////////////////////////////*/






