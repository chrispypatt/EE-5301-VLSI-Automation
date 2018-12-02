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
};

class Module {
	public:
    string name;
    vector <HyperEdge> edges; //all hyperedges for this module 
	double height, width, area, x_left, y_bottom;

	Module(){
		
	}
	Module(string _name, double _width, double _height) {
		name = _name;
		width = _width;
		height = _height;
		area = _height*_width;
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







