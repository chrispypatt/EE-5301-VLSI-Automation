//parser.h
#include <stdio.h>
#include <iostream> 
#include <fstream>
#include <string>
#include <queue>
#include <map> 

class node {
	public:
    string name,key, type; //name indicates cell type (nand, nor etc.)
    map<string,node*> inputs; //fanin nodes of this node 
	map<string,node*> outputs; //fanout nodes of this node
	bool is_out,is_in;
	int in_degree;//fanin count 
	int out_degree; //fanout count
	double height, width, area;


	node(string nm, string k,string ty, bool out, bool in) {
		name = nm;
		type = ty; 
		is_out = out;
		is_in = in;
		in_degree = 0;
		out_degree = 0;
		height = 1.0;
		width = 0.0;
		key = k;
	}

	void setDimensions(double w, double h){
		width = w;
		height = h;
		area = w*h;
	}

};

class Circuit {
	map<string,int> gate_id = {
		{"NAND", 0},
		{"NOR", 1},
		{"XOR", 2},
		{"AND", 3},
		{"OR", 4},
		{"XNOR", 5},
		{"BUFF", 6},
		{"NOT", 7},
		{"INP", 8},
		{"OUTP", 9}
	};

	public:
	queue <node*> nodes_queue;
	map <string,node*> nodes;
	double total_gates_area;
	Circuit(){
	}
	void addNode(string key, node *n){
		nodes[key] = n;
		nodes_queue.push(n);
	}
	void parseNetlist(string file_name);
	void createNode(string file_line, int i);
	void setFanInOut(string key, vector<string> fan_in_keys);
	void setGateWidths();
	void writeCktInfo();
};







