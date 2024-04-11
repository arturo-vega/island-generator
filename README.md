My first real project after learning C. Took a few weeks and a lot of frustration but I am fairly happy with it.

This program generates an island using Perlin Noise function and displays them on the terminal.

For this to work properly you need to be using Linux or OSx and have a graphical terminal capable of displaying color text.

To compile go to the director the C file is in and type 'gcc islandgenerator.c -lncurses -o islandgenerator' then you should be able to run it.

Note: It's very slow to generate the map because of the river generation which puts a random starting location on the map for a river source,
then tries to find a termination point (that point either being another river tile or an ocean tile) using a search function. This can take
a few minutes. If you want to skip this just comment out where the river generation function is called in main.
