//parser.h
#include <iostream> 
#include <fstream>
#include <string>
#include <queue>
#include <map> 
#include <boost/algorithm/string.hpp>

using namespace std;

class HyperEdge{
	public:
	int degree;
	vector <string> edge_modules; //vector of modules connected by this hyper-edge
	HyperEdge(int _degree, vector<string> _edge_modules){
		degree = _degree;
		edge_modules = _edge_modules;
	}
	HyperEdge(const HyperEdge& copy){
		degree = copy.degree;
		edge_modules = copy.edge_modules;
	}
};

class Module {
	public:
    string name;
	double height, width, area, x, y;

	Module(){
	}
	Module(string _name, double _width, double _height) {
		name = _name;
		width = _width;
		height = _height;
		area = _height*_width;
	}
};

class Circuit {
	public:
	queue<string> module_keys;
	map<string,Module*> modules;
	int module_count, edge_count;
	double total_module_area;
	vector<HyperEdge> edges; //all hyperedges for this module 

	Circuit(){}
	Circuit(const Circuit& copy){
		module_keys = copy.module_keys;
		modules = copy.modules;
		module_count = copy.module_count;
		edge_count = copy.edge_count;
		total_module_area = copy.total_module_area;
		edges = copy.edges;
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







