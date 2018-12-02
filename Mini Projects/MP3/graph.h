#include <map>
#include <list>
#include <stack> 
#include <string>
#include <limits>

using namespace std;

#define DOUBLE_MAX numeric_limits<double>::max()

class Node{//a node for our DAGs
	public: 
	string name;
    double weight; 
	Node(string _name, double _weight){ 
		name = _name; 
		weight = _weight; 
	} 
};

class Graph { 
    int node_count;
    map<string,list<Node> > adj_list;

    void topologicalSort(string v, map<string,bool>& visited, stack<string>& Stack); 

	public: 
	map<string, bool> has_incoming;

	Graph(){}
    Graph(int size){
		node_count = size;
	}

    void addEdge(string u, string v, double weight); 
    map<string,double> longestPath(string s); 
};