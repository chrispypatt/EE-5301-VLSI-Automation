#!/bin/bash
make
for filename in fpFiles/*.fp; do
    [ -e "$filename" ] || continue
	./placement "${filename#/}" -a
    ./placement "${filename#/}" -w
    ./placement "${filename#/}" -c

    # ... rest of the loop body
done
