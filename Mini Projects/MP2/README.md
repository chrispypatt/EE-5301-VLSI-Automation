# Mini Project #2
## Description

### Phase 1
The task of phase 1 was to read in a netlist from a .bench file and placing all the gates randomly onto a chip. The implementation of parsing the .bench was similar to project 1 where the gates are stored in STL maps and vectors to reference the data. After parseing the netlist,the gate nodes were placed randomly onto a chip of a specific height and width. 

The rows of the chip are implemented using deque to store the placed gates and a variable to track the current width of a row. The full chip is comprised of an array of rows. Following placement, the HPWL was calculated for each gate using the bottom left x,y position of each gate in a net. 

To compile and run this parser program, begin with the the command 'make'
To run the program, use the following command:
`./placement <benchmark_file>` 

Alternatively, you can use the included bash script `./test.sh` which will make the placement executable and then call the program for each .bench file found in the input folder. 

The program calculates and outputs the chip area, chip dimensions, and total HPWL of all the gates. Also included in the output is a a visual representation of what gates are placed in each row. The output can be found in the folder 'output' created during execution. The files are saved in the format `'chip_details_inputFileName.txt'`.

### Phase 2
TBD


### Directory Organization:

* README.md
	- This project's readme.
* Makefile
	- Make file to compile or clean directory.
* parser.cpp
	- Implementation of parser to read and parse a '.bench' file and initialize node data structures, and printing out file information.
* parser.h
	- Definitions for parser including variable definitions as well as node class definitions.
* placement.cpp
	- Implementation of chip initialization, gate placement, and simulated annealing.
* placement.h
	- Definitions for chip data structures. 
* helpers.cpp
	- Helper functions for parsing strings (spliting and finds) and converting vectors from string to double. 
* helpers.h
	- Definitions for helper functions.
* input
	- Sample circuit files to be parsed.
* output
	- Folder for output files containing chip details.
* EE5301_PA2-2.pdf
	- Project description.
* test.sh
	- Bash script to run the placement algorithm on ever bench file supplied in the input directory.

