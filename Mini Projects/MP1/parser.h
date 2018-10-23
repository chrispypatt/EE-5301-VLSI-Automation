//parser.h
#include <stdio.h>
#include <iostream> 
#include <fstream>
#include <string>
#include <vector> 
#include <list> 







class node {
	public:
    string name, outname, type; //name indicates cell type (nand, nor etc.), outname denotes the output wire name
    double Cload;//load cap of this node
	double slack,cell_delay; //values for back traversal
	double required_input_time; //Time input nodes must report to this node by           
    map<string,node*> inputs; //fanin nodes of this node 
	map<string,node*> outputs; //fanout nodes of this node
	bool is_out,is_in, output_ready;
    vector<double> Tau_in; //vector of input slews (for all inputs to the gate), to be used for STA
    vector<double> inp_arrival; //vector of input arrival times for input transitions (ignore rise or fall)
	
	map<string,double> outp_arrival; //vector of output arrival times, outp_arrival= inp_arrival + cell_delay; cell_delay will be calculated from NLDM

	vector<double> tau_outs; //vector of Taus,tau will be calculated from NLDM
    double max_out_arrival; //arrival time at the output of this gate using max on (inp_arrival + cell_delay)
	double Tau_out; 
	int in_degree;//keep track of STA dependencies
	int out_degree; //keep track of STA dependencies
	node() {
		name = "";
		type = ""; 
		in_degree = 0;
		out_degree = 0;
		is_out = false;
		is_in = false;
		output_ready = false;
	};
	node(string n, string ty, bool out, bool in) {
		name = n;
		type = ty; 
		is_out = out;
		is_in = in;
		in_degree = 0;
		out_degree = 0;
		output_ready = false;
		Cload = 0;
		slack = -1;
	}
};

class LUT {
	public:
    vector<string> Allgate_name; //all cells defined in the LUT
    map<string, vector<vector<double> > > All_delays; //array of tables (2D array) corresponding to each cell- may use a better representation like map<string, double**> which will map the cell name in string to the corresponding delay LUT
    map<string, vector<vector<double> > > All_slews; //same as All_delays, except it is now for the output slew values
    map<string, vector<double> > Cload_vals; //corresponds to the 2nd index in the LUT
    map<string, vector<double> >  Tau_in_vals; //corresponds to the 1st index in the LUT- check definition in the LUT template provided at the beginning of the LUT dfinition
	map<string, double>  Cap_in; //input capacitance
    void assignarrays(string); //function to pass the NLDM file name from which the above arrays can be populated
};

LUT lut;
map <string,node*> nodes;

map<string,double> calculateOutArrivals(node* thisnode);

vector<double> calculateOutTaus(node* thisnode);
double getCell(node* thisnode,double tau_in, int type);
string getLUTKey(node* thisnode);
vector<node*> findCritPath();

void createnodes(string bench_file);
void readLine_ckt(string fileLine,int i);
void writeCktInfo();
void writeLUTInfo(char *arg);
void writeSTA(double circuit_delay, vector<node*> critpath);

double forwardTraverse();
void findSlack(double required_time);




