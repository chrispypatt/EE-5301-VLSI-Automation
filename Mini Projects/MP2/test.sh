#!/bin/bash
make
for filename in input/*.bench; do
    [ -e "$filename" ] || continue
	./placement "${filename#/}" 

    # ... rest of the loop body
done
