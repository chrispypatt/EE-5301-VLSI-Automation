#!/bin/bash
make
for filename in fpFiles/*.fp; do
    [ -e "$filename" ] || continue
	./placement "${filename#/}" 

    # ... rest of the loop body
done
