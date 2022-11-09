#!/bin/bash

src="./src"
solution_src="$src/solution$1.c"
solution_dest="solution$1.txt"

gcc -o solution $solution_src 
./solution | ./hex2raw | cat > $solution_dest
rm solution