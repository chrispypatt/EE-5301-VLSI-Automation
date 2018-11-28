//parser.h
#include <stdio.h>
#include <iostream> 
#include <fstream>
#include <string>
#include <queue>
#include <map> 

class HyperEdge{
	public:
	int degree;
	vector <string> edge_modules; //vector of modules connected by this hyper-edge
	HyperEdge(int d, vector<string> e){
		degree = d;
		edge_modules = e;
	}
};

class Module {
	public:
    string name;
    vector <HyperEdge> edges; //all hyperedges for this module 
	double height, width, area;

	Module(){
		
	}
	Module(string n, double w, double h) {
		name = n;
		width = w;
		height = h;
		area = h*w;
	}
	void addHyperEdge(HyperEdge edge){
		edges.push_back(edge);
	}
};

class Circuit {
	public:
	queue<string> module_keys;
	map<string,Module*> modules;
	int module_count, edge_count;
	double total_module_area;

	Circuit(){
	}
	void addModule(string k, Module* b){
		modules.insert(pair<string, Module*>(k,b));
		module_keys.push(k);
	}
	void parseNetlist(string file_name);
	double createModule(string file_line);
	void createEdge(string file_line);
	void writeCktInfo();
};







