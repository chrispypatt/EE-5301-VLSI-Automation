#include <stdio.h>
#include <string>
#include <vector> 
#include <map> 
#include <iterator> 

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

    }else{
		cerr << endl;
		cerr << "Usage: " << argv[0] << " read_ckt bench_file" << endl;
		cerr << "		" << "or" << endl;
		cerr << "	" << argv[0] << " read_nldm argument lib_file" << endl;
		cerr << endl;
		return 1;
	}

    return(0);
}

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
			nodes[key] = node("INP-"+key, "INP", false, true); 
			break;
		case 2:
			key = fileLine.substr(first+1,last-first-1); //get n#
			//update node or create one if does not exist
			if ( nodes.find(key) == nodes.end() ) {
				//node not found. New node to map
				nodes[key] = node("NULL-"+key, "OUTP", true, false); 
			}else{
				nodes[key].name = name;
				nodes[key].type = type;
				nodes[key].isOut = true;
			}
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
				nodes[key] = node(name, type, false, false); 
			}else{
				nodes[key].name = name;
				nodes[key].type = type;
			}
			//get fanin and set fanout of nodesMap
			for(string inkey : fanInKeys){
				nodes[key].inputs[inkey] = &nodes[inkey];
				nodes[inkey].outputs[key] = &nodes[key];
			} 
	}
}

void writeLUTInfo(char *arg){
	string file;
	if(strcmp(arg,"delays")==0){
		file = "delay_LUT.txt";
	}else{
		file = "slew_LUT.txt";
	}
	ofstream newFile(file);


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

}

void writeCktInfo(){
	map <string, int> counts;

	int inCount = 0, outCount = 0;

	ofstream newFile("ckt_details.txt");
	if(!newFile.is_open()){
		cout << "Unable to open or create output file" << endl;
		exit(1); 
	}

	//Print details to file
	for (auto node: nodes) {
		if (node.second.isIn){
			inCount++;
		}else{
			if ( counts.find(node.second.type) == counts.end() ) {
				//gate not found. Add new gate count
				counts[node.second.type] = 1; 
			}else{
				counts[node.second.type]++;
			}
		}
		if (node.second.isOut){ //can be output and another gate
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
		if (node.second.isOut){
			newFile << node.second.name << ": OUTP" << endl;
		}else if (!node.second.isIn){
			newFile << node.second.name << ": ";
			const auto& lastKey = node.second.outputs.rbegin()->first;
			for (auto out: node.second.outputs) {
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
		if (!node.second.isIn){
			newFile << node.second.name << ": ";
			const auto& lastKey = node.second.inputs.rbegin()->first;
			for (auto out: node.second.inputs) {
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
}

void LUT::assignarrays(string NLDM_file){
//define the arrays to be used during STA call later.
//also helps to simply assign the arrays so that a call to this function will fetch the arrays, and you can easily print out the details of this NLDM
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



