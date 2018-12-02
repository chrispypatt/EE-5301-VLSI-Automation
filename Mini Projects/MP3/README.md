# Mini Project #3
## Description

### Phase 1
The task of phase 1 was to read in a netlist of modules from a .fp file and placing all the modules randomly onto a chip. The implementation of parsing the .fp was similar to project 1 where the gates are stored in STL maps and vectors to reference the data. This time, data is stored for modules which abstract gates into a large large block of gates. 

After parseing the netlist, the modules were placed on the chip randomly using random negative and positive loci for sequence pairs. The loci are used to create vertical and horizontal contraint graphs. When traversed using a longest path algorithm, the resulting paths are coordinates for the bottom left corner of each module. 

To compile and run this parser program, begin with the the command 'make'
To run the program, use the following command:
`./placement <fp_file>` 

you may also use the test.sh script to make and run the program for all .fp files in the fpFiles directory.

The program calculates and outputs the chip area, chip dimensions, and positive and negative loci, and coordinates of all the modules. The output can be found in the folder 'output' created during execution. The files are saved in the format `'inputFileName_Christopher_Patterson.txt'`.

### Phase 2
TBD



### Directory Organization:

* README.md
	- This project's readme.
* Makefile
	- Make file to compile or clean directory.
* parser.cpp
	- Implementation of parser to read and parse a '.fp' file and initialize module data and circuit structures.
* parser.h
	- Definitions for parser including variable definitions as well as Module class definitions.
* placement.cpp
	- Implementation of chip initialization, Module placement using sequence pair, and simulated annealing.
* placement.h
	- Definitions for chip data structures. 
* helpers.cpp
	- Helper functions for parsing strings (spliting and finds) and converting vectors from string to double. 
* helpers.h
	- Definitions for helper functions.
* graph.cpp
	- Code to traverse a graph represented as an adjacency list. This is used with the sequence pair to create vertical and horizontal contraint graphs for module placement. Placement is done using longest path search over the graphs
* graph.h
	- Definitions for graph, topological sort, and longest path functions.
* fpFiles
	- Sample netlist files to be parsed.
* output
	- Folder for output files containing chip details.
* EE5301_PA2-2.pdf
	- Project description.
* test.sh
	- Bash script to run the placement algorithm on every fp file supplied in the fpFiles directory.

