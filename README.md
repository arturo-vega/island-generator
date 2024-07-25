![Alt text](screenshot3.png?raw=true "Screenshot")

This program uses perlin noise to create a noise map that is then converted into a character array that is printed onto the terminal. It uses color characters but can be run without them if your terminal doesn't support color characters. It attempts to generate rivers but because there is no additional simulation of the noise map for erosion and rainfall the algorithm can take sometime to complete as the generation is bruteforced, takes usually about 2-4 minutes. 

This can be run MacOS or Linux as long as you have the ncurses dependencies. If you want to run it on Windows you can use the Windows Subsystem for Linux (WSL) to run a Linux terminal environment. 

To compile this C file you need to have the ncurses dependencies first. Then enter the following command into the terminal: 
'gcc islandgenerator.c -o islandgenerator -lncurses -lm' 

