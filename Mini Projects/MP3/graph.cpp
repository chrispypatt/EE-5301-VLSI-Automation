#include "graph.h"

void Graph::addEdge(string u, string v, double weight){
	Node node(v, weight); 
    adj_list[u].push_back(node); // Add v to u's list 
}

void Graph::topologicalSort(string v, map<string,bool>& visited, stack<string>& Stack){
    visited[v] = true; 

	//topological sort for all children of this node
    list<Node>::iterator i; 
    for (i = adj_list[v].begin(); i != adj_list[v].end(); ++i) { 
        Node node = *i; 
        if (!visited[node.name]) {
            topologicalSort(node.name, visited, Stack); 
		}
    } 
    // when all children nodes of this one and their children
	//have been added to stack, push to stack
    Stack.push(v); 
}

map<string,double> Graph::longestPath(string s){
	stack<string> Stack; 
	map<string,double> distances;
	map<string, bool> visited;

	//set up structures used for longest path search
	for (auto elem: adj_list){
		string key = elem.first;
		distances[key] = 0;
		visited[key] = false;
	}
	distances[s] = 0; 

	//run topological sort
	for (auto elem: adj_list){
		string key = elem.first;
		if(!visited[key]){
			topologicalSort(key, visited, Stack);	
		}
	}

	//stack is sorted topologically, now we can do our search
	while (!Stack.empty()) { 
        // Get the next vertex from topological order 
        string u = Stack.top(); 
        Stack.pop(); 

        // Update distances of all adjacent vertices 
        list<Node>::iterator i; 
        if (distances[u] != DOUBLE_MAX) { 
            for (i = adj_list[u].begin(); i != adj_list[u].end(); ++i) {
                if (distances[i->name] < distances[u] + i->weight){
                    distances[i->name] = distances[u] + i->weight; 
				} 
			}

        } 
    } 
	return distances;
}