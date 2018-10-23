# Mini Project #1
Directory Organization:

* README.md
	- This project's readme.
* Makefile
	- Make file to compile or clean directory.
* parser.cpp
	- Implementation of parser to read in command line args, parse '.bench' or '.lib' files, assign LUT or node data structures, and printing out file information.
* parser.h
	- Definitions for parser including variable definitions as well as LUT and node class definitions. 
* helpers.cpp
	- Helper functions for parsing strings (spliting and finds) and converting vectors from string to double. 
* helpers.h
	- Definitions for helper functions.
* Benchmarks
	- Sample circuit files to be parsed.
* sample_NLDM.lib
	- Sample nldm file to be parsed.
* EE5301F18-MP1-1.pdf
	- Project description.

## Description

###Phase 1
The first phase of this project was approached using string comparisons to loop through each line of an input file and get either nldm or circuit information based on user input. Implementation relied heavily on the C++ STL using vectors and maps to keep track and associate data with each other. 

To compile and run this parser program, begin with the the command 'make'
To parse a file, you can run one of two commands:
* `./parser read_nldm argument NLDM_file.lib` to parse the nldm file specified and writes the data to either 'delay_LUT.txt' or 'slew_LUT.txt' based on the argument specified.
	* argument={“delays”, “slews”} to either delay or slew info
* `./parser read_ckt benchmark_file.bench` to parse the benchmark file specified and writes the output to 'ckt_details.txt'

###Phase 2
The second phase of this project's goal was to traverse throught the circuit provided through a user's input and print out the full circuit delay along with gate slacks and the circuit's critical path (based on output required arrival times equal to 1.1x the circuit delay). This was done using three traversals of the circuit: 
1. Forward topological traversal to calculate the maximum gate arrival times (actual arrival), output slews and total circuit delay. 
2. Backward topological traversal to calculate the required arrival time (based on total  required time 1.1x actual circuit delay) of each gate and the slack (required arrival - actual arrival). 
3. Backwards greedy traversal to select a minimum slack output gate and subsequently the minimum slack input to each gate chose from there until it finds an input gate. My algorithm just selects a random gate if slacks are the same for a given level.  

All the vital information found through these traversals are output to 'ckt_traversal.txt'. The program can be run using the command: `./parser  benchmark_file.bench`



