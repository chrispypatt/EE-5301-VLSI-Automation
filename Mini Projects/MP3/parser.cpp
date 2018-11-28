#include <stdio.h>
#include <string>
#include <cstring>
#include <queue> 
#include <map> 
#include <boost/algorithm/string.hpp>

using namespace std;

#include "helpers.h" 
#include "parser.h"

		
void Circuit::parseNetlist(string file_name){
	// cout << endl << "Parsing file: " << file_name << endl;
	string file_line;
	ifstream inFile;

	//Init any values we need to
	total_module_area = 0;  

    inFile.open(file_name);
    if (!inFile) {
        cout << "Unable to open input file" << endl;
        exit(1); 
    }

	//Read in modules
	//First line is the number of modules in net
	getline(inFile, file_line);
	boost::erase_all(file_line, "\r"); // removes \r if file is from windows machine

	module_count = stoi(file_line);
	for (int i = 0; i < module_count; i++){
		getline(inFile, file_line);
		boost::erase_all(file_line, "\r"); 
		total_module_area += createModule(file_line); 
	}

	//read in hyperedges
	getline(inFile, file_line); //dummy read to get rid of "Nets" line
	getline(inFile, file_line);
	boost::erase_all(file_line, "\r"); 
	edge_count = stoi(file_line);
	for (int i = 0; i < edge_count; i++){
		getline(inFile, file_line);
		boost::erase_all(file_line, "\r"); 
		createEdge(file_line); 
	}
    inFile.close();
	cout << "Parsing completed!" << endl;
	cout << "Module Count: " << module_count << " Hyper-edge Count: " << edge_count << endl;
}



/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*								DATA STRUCTURE INIT								  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

double Circuit::createModule(string file_line){
	double module_area;
	Module *temp_module;

	vector <string> split_line = split(file_line, " "); // <module#> <width> <height>
	temp_module = new Module(split_line[0], stod(split_line[1]), stod(split_line[2]));
	
	addModule(temp_module->name,temp_module);
	return temp_module->area;
}

void Circuit::createEdge(string file_line){
	vector <string> temp_modules;
	int degree;

	//read in all modules connected to this edge
	vector <string> split_line = split(file_line, " "); // degree <module> ... <module>
	degree = stoi(split_line[0]);

	for (int i = 1; i <= degree; i++){
		temp_modules.push_back(split_line[i]);
	}

	//create hyper-edge
	HyperEdge temp_edge(degree,temp_modules);

	//attach edge info to all interested parties
	for (int i = 0; i < degree; i++) {
		string key = temp_modules[i];
		modules[key]->addHyperEdge(temp_edge);
	}
}

/*//////////////////////END DATA STRUCTURE INIT//////////////////////////////////////*/





/*////////////////////////////////////////////////////////////////////////////////*/
/*																		  		  */			
/*						PRINT DATA TO OUTPUT FILES								  */
/*																				  */
/*////////////////////////////////////////////////////////////////////////////////*/

// void Circuit::writeCktInfo(){
// 	map <string, int> counts;

// 	int inCount = 0, outCount = 0;

// 	ofstream newFile("ckt_details.txt");
// 	if(!newFile.is_open()){
// 		cout << "Unable to open or create output file" << endl;
// 		exit(1); 
// 	}

// 	newFile << "-----------------------------------------------------" << endl;
// 	//Print details to file
// 	for (auto node: nodes) {
// 		if (node.second->is_in){
// 			inCount++;
// 		}else if (node.second->is_out){ //can be output and another gate
// 			outCount++;
// 		}else{
// 			if ( counts.find(node.second->type) == counts.end() ) {
// 				//gate not found. Add new gate count
// 				counts[node.second->type] = 1; 
// 			}else{
// 				counts[node.second->type]++;
// 			}
// 		}

// 	} 

// 	//print counts
// 	newFile << inCount << " primary inputs" << endl;
// 	newFile << outCount << " primary outputs" << endl;
// 	for (auto count: counts) {
// 		newFile << count.second << " " << count.first << " gates" << endl;
// 	} 

// 	//print fanin and fanout
// 	newFile << endl << endl;
// 	newFile << "Fanout..." << endl;
// 	queue <node*> temp_queue = nodes_queue; //copy the original queue to the temporary queue
// 	while (!temp_queue.empty()){
// 		node *n = temp_queue.front();
// 		if (!(n->is_in || n->is_out)){
// 			newFile << n->name << ": ";
// 			const auto& lastKey = n->outputs.rbegin()->first;
// 			for (auto out: n->outputs) {
// 				newFile << out.second->name;
// 				if(out.first != lastKey){
// 					newFile << ",";
// 				}
// 				newFile << " ";
// 			}
// 			newFile << endl;
// 		}
// 		temp_queue.pop();
// 	} 

// 	newFile << endl << endl;
// 	newFile << "Fanin..." << endl;
// 	temp_queue = nodes_queue; //copy the original queue to the temporary queue
// 	while (!temp_queue.empty()){
// 		node *n = temp_queue.front();
// 		if (!(n->is_in || n->is_out)){
// 			newFile << n->name << ": ";
// 			const auto& lastKey = n->inputs.rbegin()->first;
// 			for (auto out: n->inputs) {
// 				newFile << out.second->name;
// 				if(out.first != lastKey){
// 					newFile << ",";
// 				}
// 				newFile << " ";
// 			}
// 			newFile << endl;
// 		}
// 		temp_queue.pop();
// 	}
// 	newFile << "-----------------------------------------------------" << endl;
// }

/*//////////////////////END PRINT OUT////////////////////////////////////////////*/






