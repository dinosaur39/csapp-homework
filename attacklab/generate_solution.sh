#!/bin/bash

src="./src"
solution_src="$src/solution$1.c"
solution_dest="solution$1.txt"
solution_hex="$src/solution$1_hex.txt" 
gcc -o solution $solution_src 
./solution > $solution_hex
./hex2raw < $solution_hex | cat > $solution_dest
rm solution