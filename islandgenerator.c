#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <ncurses.h>

#define MAX_QUEUE_SIZE 10000


int starty = 0;
int startx = 0;
int SEED = 0;
int hash[] = {
                    208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
                    185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
                    9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
                    70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
                    203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
                    164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
                    228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
                    232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
                    193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
                    101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
                    135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
                    114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219
                    };
char *choices[] = {
	"Choice 1",
	"Choice 2",
	"Choice 3",
	"Choice 4",
};
static int n_choices = sizeof(choices) / sizeof(char *);
struct coordinates {
	int y, x;
};

//map_height = win_dim.rheight;
//map_width = win_dim.rwidth;
// Struct of terminal and sub-window dimensions
struct win_dimensions
{
    int lwidth;
    int lheight;
    int rstarty;
    int rstartx;
    int rwidth;
    int rheight;
    int bheight;
    int bwidth;
    int bstartx;
    int bstarty;
};
typedef struct win_dimensions WIN_DIMENSIONS;
typedef struct player_pos PLAYER_POS;
typedef struct coordinates COORDINATES;

struct prev{
    int x;
    int y;
    int ID;
    int parent;
};

struct QueueNode{
    int x;
    int y;
};
struct Queue {
    struct QueueNode nodes[MAX_QUEUE_SIZE];
    int front;
    int rear;
};
int game_loop = 1;
int highlight = 1;
int first_init = 1;
int number_of_prints = 0;

COORDINATES pos = { .y = 0, .x = 0 };
WIN_DIMENSIONS win_dim;
WINDOW *left_win, *right_win, *bottom_win;

// Functions for Perlin Noise by nowl on github
int noise2(int x, int y);
float lin_inter(float x, float y, float s);
float smooth_inter(float x, float y, float s);
float noise2d(float x, float y);
float perlin2d(float x, float y, float freq, int depth);


WINDOW *create_newwin(int height, int width, int starty, int startx); // Creates the new windows
void init_windows(WINDOW **left_win, WINDOW **right_win, WINDOW **bottom_win, char **tile_map_ptr); // Initializes the windows and calls create_newwin
void update_windows(WINDOW **left_win, WINDOW **right_win, WINDOW **bottom_win, char **tile_map_ptr); // Updates windows at the end of game loop
void print_menu(WINDOW *left_win); // Function to create menu options on right window
void menu_highlight(WINDOW *left_win); // Function to highlight selection on right window
void generate_height_map(float **height_map_ptr, float frequency, float depth, int noise_map_height, int noise_map_width); // Base height map, used by all other maps
void height_map_cleanup(float **height_map_ptr, float water_threshold, int noise_map_height, int noise_map_width); // Will remove isolated water and land tiles when used
void generate_tile_map(char **tile_map_ptr, float **height_map_ptr, int noise_map_width, int noise_map_height, float water_threshold); // Generates the map to display on right window
void pd_algorithm(float **height_map_ptr, int noise_map_height, int noise_map_width, int water_threshold, float erosion_amount); // Smoothes the map, voodoo, don't really know how useful it is
void generate_rivers(float **height_map_ptr, char **tile_map_ptr, int noise_map_height, int noise_map_width); // Function that places rivers, uses bfs_river to generate them
void display_tile_map(WINDOW *right_win, char **tile_map_ptr, COORDINATES *view_port_ptr, int noise_map_height, int noise_map_width); // Function to display the tile map
void handle_input(WINDOW *left_win, int input, int noise_map_height, int noise_map_width, COORDINATES *view_port_ptr, int scroll_speed); // Gets user input for character and feeds it into display_tile_map
void update_terminal_dimensions(void); // Gets dimensions of terminal and recalculates subwindows

void print_to_bottom_window(WINDOW *bottom_win, char *str, int input);
void print_to_stdscr(int rows, int cols, char *str);

int main(void)
{
    int noise_map_height = 500;
    int noise_map_width = 500;
    COORDINATES view_port = {.y = noise_map_height / 2, .x = noise_map_width / 2};
    COORDINATES *view_port_ptr = &view_port;
	int old_rows, old_cols, rows, cols;
	int input;
	int get_input = 1;
    int number_of_prints = 1;
    float erosion_amount = 1;
    int scroll_speed = 2; // How many tiles a key press will advance the screen in the coresponding direction
    float water_threshold = 0.20; // Affects height of water and also the height of everything else in proportion
    float frequency = 20; // Lower the numbe rhte more 'zoom'
    float depth = 3; // Lower number = more gradual changes in height map


    // Initialize the ncurses library
    initscr();
    start_color();
	noecho();
	cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    keypad(left_win, TRUE);

    //Init color pairs
    init_pair(1, COLOR_BLUE, COLOR_BLACK);    // Water
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Sand/Dirt
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Grass

	// Get terminal size and save it
    getmaxyx(stdscr, rows, cols);
	old_rows = rows;
	old_cols = cols;

    // Create a dynamically allocated array of characters with the dimensions of the world
    char **tile_map_ptr = malloc(noise_map_height * sizeof(char*));
    for (int i = 0; i < noise_map_height; i++) {
        tile_map_ptr[i] = malloc(noise_map_width * sizeof(char));
    }

    // Create a dynamically allocated array of floats with the dimensions of the world
    float **height_map_ptr = malloc(noise_map_height * sizeof(float*));
    for (int i = 0; i < noise_map_height; i++) {
        height_map_ptr[i] = malloc(noise_map_width * sizeof(float));
    }

    // Initial player start, add on to this later
    update_terminal_dimensions();
    pos.y = view_port.y + win_dim.rheight / 2;
    pos.x = view_port.x + win_dim.rwidth / 2;

    // Generate the map for the first time
    print_to_stdscr(rows / 2, cols / 2, "Generating Height Map");
    generate_height_map(height_map_ptr, frequency, depth, noise_map_height, noise_map_width);
    height_map_cleanup(height_map_ptr, water_threshold, noise_map_height, noise_map_width);

    print_to_stdscr(rows / 2, cols / 2, "Smoothing Height Map");
    pd_algorithm(height_map_ptr, noise_map_height, noise_map_width, water_threshold, erosion_amount);

    print_to_stdscr(rows / 2, cols / 2, "Genereating Tile Map");
	generate_tile_map(tile_map_ptr, height_map_ptr, noise_map_width, noise_map_height, water_threshold);

    print_to_stdscr(rows / 2, cols / 2, "Placing Rivers");
    generate_rivers(height_map_ptr, tile_map_ptr, noise_map_height, noise_map_width);

	// Initialize windows and menu for first time
    display_tile_map(right_win, tile_map_ptr, view_port_ptr, noise_map_height, noise_map_width);
	init_windows(&left_win, &right_win, &bottom_win, tile_map_ptr);

	// Main game loop
    while(game_loop == 1)
	{
        // Get terminal size and refresh windows
        getmaxyx(stdscr, rows, cols);

        if (rows != old_rows || cols != old_cols)
        {
            // The terminal window has been resized, so re-initialize the windows
            // with the new dimensions
			old_rows = rows;
			old_cols = cols;
            init_windows(&left_win, &right_win, &bottom_win, tile_map_ptr);
            display_tile_map(right_win, tile_map_ptr, view_port_ptr, noise_map_height, noise_map_width);
            update_windows(&left_win, &right_win, &bottom_win, tile_map_ptr);
		}

        else
        {
            display_tile_map(right_win, tile_map_ptr, view_port_ptr, noise_map_height, noise_map_width);
			update_windows(&left_win, &right_win, &bottom_win, tile_map_ptr);
        }

		get_input = 1;
		// Get input from user
		while(get_input == 1)
		{
			input = getch();

			if (input == 10){
                print_to_bottom_window(bottom_win, "Key pressed: ", input);
				menu_highlight(left_win);
            }
			else
				handle_input(left_win, input, noise_map_height, noise_map_width, view_port_ptr, scroll_speed);


			get_input = 0;
		}
	}

	free(tile_map_ptr);
    free(height_map_ptr);
    wgetch(bottom_win);
    endwin();

    return 0;
}

void init_windows(WINDOW **left_win, WINDOW **right_win, WINDOW **bottom_win, char **tile_map_ptr)
{
     // Get the number of rows and columns in the terminal window
    update_terminal_dimensions();

    // Create the windows
    *left_win = create_newwin(win_dim.lheight, win_dim.lwidth, starty, startx);
    *right_win = create_newwin(win_dim.rheight, win_dim.rwidth, win_dim.rstarty, win_dim.rstartx);
    *bottom_win = create_newwin(win_dim.bheight, win_dim.bwidth, win_dim.bstarty, win_dim.bstartx);
    //Allows scrolling on these windows
    scrollok(*bottom_win, 1);
    scrollok(*right_win, 1);

	print_menu(*left_win);

    refresh();
    wnoutrefresh(*left_win);
    wnoutrefresh(*right_win);
    wnoutrefresh(*bottom_win);
    doupdate();
}

void update_windows(WINDOW **left_win, WINDOW **right_win, WINDOW **bottom_win, char **tile_map_ptr)
{
    // Get the number of rows and columns in the terminal window
    update_terminal_dimensions();

	// Create stuff on windows
	print_menu(*left_win);


    // Refresh the screen to display the windows
    refresh();
    wnoutrefresh(*left_win);
    wnoutrefresh(*right_win);
    wnoutrefresh(*bottom_win);
    doupdate();

}

WINDOW *create_newwin(int height, int width, int starty, int startx)
{
	WINDOW *local_win;

	local_win = newwin (height, width, starty, startx);
	box(local_win, 0, 0);

	return local_win;
}

void print_to_bottom_window(WINDOW *bottom_win, char *str, int input)
{
    // Move the first waddstr down by one for the first print
    if(number_of_prints == 1)
    {
        wmove(bottom_win, 1, 1);
        wprintw(bottom_win, "%s", str);
        wprintw(bottom_win, "%d\n", input);
        box(bottom_win, 0, 0);
    }

        wmove(bottom_win, number_of_prints, 1);
        wprintw(bottom_win, "%s", str);
        wprintw(bottom_win, "%d\n", input);
        box(bottom_win, 0, 0);

    // Scroll the bottom window up by one line
    //14 lines and then a box draws and the first letter is cut off
    number_of_prints += 1;
    if(number_of_prints > 200)
        number_of_prints = 20;

}

void print_to_stdscr(int rows, int cols, char *str)
{
    clear();
    mvprintw(rows / 2, cols / 2, "%s", str);
    refresh();
}

void handle_input(WINDOW *left_win, int input, int noise_map_height, int noise_map_width, COORDINATES *view_port_ptr, int scroll_speed)
{
    // Check the character and update the position of the character
    // on the screen accordingly
    switch(input)
    {
        case KEY_UP: //up
            // Don't change character pos if at top of the screen
            if(pos.y == 1)
                break;
            // Let the character move up but without scrolling the screen if near top
            if((win_dim.rheight / 2) - pos.y + 2 >= 0)
            {
                pos.y -= scroll_speed;
                break;
            }
            // Let character move up but without scrolling the screen if near bottom
            if(noise_map_height - pos.y <= (win_dim.rheight / 2))
            {
                pos.y-=scroll_speed;
                break;
            }
            else
            {
                view_port_ptr->y-=scroll_speed;
                pos.y-=scroll_speed;
                break;
            }

        case KEY_DOWN: //down
            // Don't change character pos if at bottom of screen
            if(pos.y == noise_map_height - 2)
                break;
            // Let character move down but without scrolling the screen if near bottom
            if((win_dim.rheight / 2) + pos.y + scroll_speed >= noise_map_height)
            {
                pos.y+=scroll_speed;
                break;
            }
            // Let character move down but without scrolling the screen if near top
            if(pos.y <= (win_dim.rheight / 2))
            {
                pos.y+=scroll_speed;
                break;
            }
            else
            {
                view_port_ptr->y+=scroll_speed;
                pos.y+=scroll_speed;
                break;
            }

        case KEY_LEFT: //left
            if(pos.x  == 1)
                break;
            if((win_dim.rwidth / 2) - pos.x + scroll_speed >= 0)
            {
                pos.x-= scroll_speed;
                break;
            }
            if(noise_map_width - pos.x <= (win_dim.rwidth / 2))
            {
                pos.x-=scroll_speed;
                break;
            }
            else
            {
            view_port_ptr->x-=scroll_speed;
            pos.x-=scroll_speed;
            break;
            }

        case KEY_RIGHT: //right
            if(pos.x == noise_map_width - 3)
                break;
            if((win_dim.rwidth / 2) + pos.x + 1 >= noise_map_width)
            {
                ++pos.x;
                break;
            }
            // Let character move down but without scrolling the screen if near top
            if(pos.x <= (win_dim.rwidth / 2))
            {
                pos.x+=scroll_speed;
                break;
            }
            else{
                view_port_ptr->x+=scroll_speed;
                pos.x+=scroll_speed;
                break;
            }

        default:
            break;
    }
    //print_to_bottom_window(bottom_win, "value of input = ", input);
    print_to_bottom_window(bottom_win, "pos.char_x =", pos.x);
    print_to_bottom_window(bottom_win, "pos.char_y =", pos.y);

}

// Doesn't work right, need to fix
void menu_highlight(WINDOW *left_win)
{
	int choice = 0;
    int ch = wgetch(left_win);
    while(1){
		switch(ch)
		{
			case KEY_UP:
				if(highlight == 1)
					highlight = n_choices;
				else
					highlight -= highlight;
				break;
			case KEY_DOWN:
				if(highlight == n_choices)
					highlight = 1;
				else
					highlight += highlight;
				break;
			case 10:
				choice = highlight;
				break;
            default:
                break;
		}
    print_menu(left_win);
    print_to_bottom_window(bottom_win, "Finished menu_highlight loop with =", ch);
    refresh();
    wrefresh(left_win);
    if(choice != 0)
        break;
    }
}

void print_menu(WINDOW *left_win)
{
	int x, y, i;

	x = 6;
	y = 6;
	box(left_win, 0, 0);
	for (i = 0; i < n_choices; ++i)
	{
		if (highlight == i + 1) //Highlight the present choice
		{
			wattron(left_win, A_REVERSE);
			mvwprintw(left_win, y, x, "%s", choices[i]);
			wattroff(left_win, A_REVERSE);
		}
		else
			mvwprintw(left_win, y, x, "%s", choices[i]);
		++y;
	}
    print_to_bottom_window(bottom_win, "Finished print_menu function with highlight = ", highlight);

}

// Creates the height map from the noise function
void generate_height_map(float **height_map_ptr, float frequency, float depth, int noise_map_height, int noise_map_width)
{
    srand(time(0));
    int seed = 1;
    SEED = seed;

    for (int i = 0; i < noise_map_height; i++)
    {
        for (int j = 0; j < noise_map_width; j++)
        {
            float x = (float)j / noise_map_width;
            float y = (float)i / noise_map_height;
            float n = perlin2d(x, y, frequency, depth);
            height_map_ptr[i][j] = n;
        }
    }
}

// This function is AcountS but it gets rid of single or two cells that are surrounded by above or below water threshold cells when they are the opposite
// Not even sure it's useful but I'm keeping it because it was annoying to write, can be gotten rid of with no problems
void height_map_cleanup(float **height_map_ptr, float water_threshold, int noise_map_height, int noise_map_width)
{
    for (int i = 1; i < noise_map_height - 1; ++i)
    {
        for (int j = 1; j < noise_map_width - 1; ++j)
        {
            int opposite_count = 0;

            for (int y = -1; y <= 1; y+=2)
            {
                if ((height_map_ptr[i+y][j] < water_threshold && height_map_ptr[i][j] > water_threshold) || (height_map_ptr[i-y][j] > water_threshold && height_map_ptr[i][j] < water_threshold))
                {
                    opposite_count++;

                }
            }

            for (int x = -1; x <= 1; x+=2)
            {
                if ((height_map_ptr[i][j-x] < water_threshold && height_map_ptr[i][j] > water_threshold) || (height_map_ptr[i][j+x] > water_threshold && height_map_ptr[i][j] < water_threshold))
                {
                    opposite_count++;
                }
            }

            if (opposite_count == 4)
                height_map_ptr[i][j] = height_map_ptr[i-1][j];
        }
    }
}



void generate_tile_map(char **tile_map_ptr, float **height_map_ptr, int noise_map_width, int noise_map_height, float water_threshold)
{
    // Generate the Perlin noise values for each tile in the map
    for (int i = 0; i < noise_map_height; i++)
    {
        for (int j = 0; j < noise_map_width; j++)
        {
            float n = height_map_ptr[i][j];

            // Assign a character to the tile based on the noise value
            if (n < water_threshold)
                tile_map_ptr[i][j] = 'w'; // Water

            else if (n < water_threshold + 0.05)
                tile_map_ptr[i][j] = '.'; // Lowlands

            else if (n < water_threshold + 0.15)
                tile_map_ptr[i][j] = ','; // Plains

            else if (n < water_threshold + 0.25)
                tile_map_ptr[i][j] = ';'; // Highlands

            else if (n < water_threshold + 0.35)
                tile_map_ptr[i][j] = ':'; // Hills

            else if (n < water_threshold + 0.55)
                tile_map_ptr[i][j] = '^'; // Mountains

            else
                tile_map_ptr[i][j] = '#'; // Peaks
        }
    }
}

// Planchon-Darboux algorithm for creating a smooth flow toward the edges of the map
// Not sure this really does much
void pd_algorithm(float **height_map_ptr, int noise_map_height, int noise_map_width, int water_threshold, float erosion_amount)
{
    int iterations = 5;
    float lambda = .5;

    // Perform the iterations
    for (int k = 0; k < iterations; k++)
    {
        for (int i = 1; i < noise_map_height - 1; i++)
        {
            for (int j = 1; j < noise_map_width - 1; j++)
            {

                float dzdx = (height_map_ptr[i][j+1] - height_map_ptr[i][j-1]) / 2.0;
                float dzdy = (height_map_ptr[i+1][j] - height_map_ptr[i-1][j]) / 2.0;
                float slope = sqrt(dzdx*dzdx + dzdy*dzdy);
                float aspect = atan2(dzdy, dzdx);
                float sediment = erosion_amount * slope * slope;

                // find the lowest neighboring cell
                int minX = j, minY = i;
                float minHeight = height_map_ptr[i][j];
                for (int x = -1; x <= 1; x++) {
                    for (int y = -1; y <= 1; y++) {
                        if (height_map_ptr[i+y][j+x] < minHeight) {
                            minX = j+x;
                            minY = i+y;
                            minHeight = height_map_ptr[i+y][j+x];
                        }
                    }
                }

                // move the sediment to the lowest neighboring cell
                height_map_ptr[minY][minX] += sediment;
                height_map_ptr[i][j] -= sediment;
                float newHeight = height_map_ptr[i][j] + lambda * slope * sin(aspect);
                height_map_ptr[i][j] = newHeight;
            }
        }
    }

}

// Creates a queue and returns its address
struct Queue *createQueue(void)
{
    struct Queue *q = (struct Queue *)malloc(sizeof(struct Queue));
    q->front = -1;
    q->rear = -1;
    return q;
}

// Returns true is the queue is full
bool isFull(struct Queue *q){
    return q->rear == MAX_QUEUE_SIZE - 1;
}

// Returns true if the queue is empty, false otherwise
bool isEmpty(struct Queue *q) {
    return q->front == -1;
}

// Adds a point to the rear of the queue
void enQueue(struct Queue *q, int x, int y) {

    q->rear++;
    q->nodes[q->rear].x = x;
    q->nodes[q->rear].y = y;

    if (q->front == -1)
        q->front = q->rear;
}

// Removes a point from the front of the queue
struct QueueNode deQueue(struct Queue *q) {
    if (q->front == -1){
        struct QueueNode empty = {-1,-1};
        return empty;
    }
    else
    {
        struct QueueNode value = q->nodes[q->front];

        if(q->front == q->rear){
            q->front = -1;
            q->rear = -1;
        }
        else
            q->front++;
        return value;
    }
}

//Clears the river map back to 'f' but any char besides t or p would be fine
void clear_river_map(char **river_map_ptr, int noise_map_height, int noise_map_width)
{
    for (int i = 10; i < noise_map_height - 10; ++i){
        for (int j = 10; j < noise_map_width - 10; ++j)
        {
            river_map_ptr[i][j] = 'f';
        }
    }
}

// Checks if a river starting tile has water tiles or river tiles in a certain area around it
bool good_river(char **tile_map_ptr, int noise_map_height, int noise_map_width, int y, int x)
{
    for (int i = -20; i < 20; ++i){
        for (int j = -20; j < 20; ++j){
            if (tile_map_ptr[y+i][x+j] == 'w' )
                return false;
        }
    }
    for (int i = -15; i < 15; ++i){
        for (int j = -15; j < 15; ++j){
            if (tile_map_ptr[y+i][x+j] == '~' )
                return false;
        }
    }
    return true;
}


// Breadth First Search Algorithm for finding path between river source and terminiation [GONE WRONG[GONE SEXUAL]]
int bfs_river(char **river_map_ptr, int noise_map_height, int noise_map_width, int beginy, int beginx, int endy, int endx)
{
    int x, y, nx, ny;
    int nodes = 0;
    bool reconstruct_path = false;
    struct QueueNode currnode;
    struct prev reconsctnode;

    int dx[8] = { -1, 1, 0, 0, -1, 1, -1, 1 };
    int dy[8] = { 0, 0, -1, 1, -1, 1, 1, -1 };

    //Create integer array to hold visited and not visited points
    int **bfs_graph_ptr = malloc(noise_map_height * sizeof(int*));
    for (int i = 0; i < noise_map_height; ++i){
    bfs_graph_ptr[i] = malloc(noise_map_width * sizeof(int));}

    // Set all places that don't have a river tile to -1 so that they aren't checked as nodes
    // Set all places with a river tile 't' to 0 to show they haven't been visited
    for (int i = 10; i < noise_map_height - 10; ++i){
        for(int j = 10; j < noise_map_width - 10; ++j){
            bfs_graph_ptr[i][j] = -1;
            if (river_map_ptr[i][j] == 't' || river_map_ptr[i][j] == 's') {
                bfs_graph_ptr[i][j] = 0;
                ++nodes;
            }
        }
    }

    // Struct to hold nodes visited and their parents
   struct prev visited[nodes];

    // Initialize prev with all negatives
    for (int i = 0; i < nodes; ++i){
       visited[i].x = 0;
       visited[i].y = 0;
       visited[i].parent = 0;
    }

    // create a queue to hold points that need to be visited next
    struct Queue *r_q = createQueue();
    // enQueue starting position and mark it as visited
    enQueue(r_q, beginx, beginy);
    int count = 1;
    int parent_node = count;
    visited[count].x = beginx;
    visited[count].y = beginy;
    visited[count].parent = count;

    // Keep going until queue is empty then the loop will break 
    while (!isEmpty(r_q))
    {
        currnode = deQueue(r_q);
        
        // Break out of the loop because the path to the source of the river has been found
        if (visited[count].x == endx && visited[count].y == endy){
            reconstruct_path = true;
            break;
        }
        //Get neighbor nodes and queue them, parent node is the node whose neighbors are checked, they are
        //assigned that node as it's parent. Because of FIFO of queue the parent_node increment should match
        //with the count (IE: a node at count 2 should have a parent_node of 2)
        for (int i = 0; i < 8; ++i)
        {
            nx = currnode.x + dx[i];
            ny = currnode.y + dy[i];
           

            if (bfs_graph_ptr[ny][nx] == 0)
            {
                enQueue(r_q, nx, ny);
                visited[count+1].parent = parent_node;
                visited[count+1].x = nx;
                visited[count+1].y = ny;
                bfs_graph_ptr[ny][nx] = 1;
                ++count;
            }

        }
        ++parent_node;
    }
    
    //Reconstruct path back to start node
    int return_v = 1;
    if (reconstruct_path == true){
    river_map_ptr[beginy][beginx] = 'p';
    count = visited[count].parent;
    }

    //When the end node is reached it is just a matter of following that nodes parents until they reach 1 which is the parent_node ID of
    //where it started at the beginning of the function
    while(reconstruct_path == true)
    {
        river_map_ptr[endy][endx] = 'p';
        river_map_ptr[visited[count].y][visited[count].x] = 'p';


        if (visited[count].parent == 1){
            river_map_ptr[visited[count].y][visited[count].x] = 'p';
            reconstruct_path = false;
            return_v = 0;
        }
        else
            count = visited[count].parent;
    }
    reconstruct_path = false;
    free(bfs_graph_ptr);

    // Will return 1 if no path has been found otherwise 0
    return return_v;
}

void generate_rivers(float **height_map_ptr, char **tile_map_ptr, int noise_map_height, int noise_map_width)
{
    int num_rivers = 40; // number of rivers to generate
    int river_count = 0;
    int max_tries = 10000;
    int x = 20;
    int y = 20;
    struct QueueNode currnode;
    int beginy, beginx, endy, endx;
    bool gen_check = false;
    int counter = 0;
    bool checked[noise_map_height][noise_map_width];
    int bfs_return;

    // Initialize the checked array to avoid repeating placing rivers on the same tile if it's already been checked
    for (int i = 0; i < noise_map_height; ++i){
        for (int j = 0; j < noise_map_width; ++j){
            checked[i][j] = false;
        }
    }

    char **river_map_ptr = malloc(noise_map_height * sizeof(char*));
    for (int i = 0; i < noise_map_height; i++) {
        river_map_ptr[i] = malloc(noise_map_width * sizeof(char));
    }

    while (river_count < num_rivers)
    {

        ++counter;
        clear_river_map(river_map_ptr, noise_map_height, noise_map_width);

        x = rand() % ((noise_map_width - 75) + 1 - 75) + 75;
        y = rand() % ((noise_map_height - 75) + 1 - 75) + 75;

        // pick a random starting point for the river
        // if starting point is already water, pick another point
        while (tile_map_ptr[y][x] == '~' || gen_check == false || checked[y][x] == true)
        {
            x = rand() % ((noise_map_width - 75) + 1 - 75) + 75;
            y = rand() % ((noise_map_height - 75) + 1 - 75) + 75;
            gen_check = good_river(tile_map_ptr, noise_map_height, noise_map_width, y, x);

        }
        checked[y][x] = true;
        gen_check = false;
        // mark the starting point as a river
        endy = y;
        endx = x;

        // create a queue to hold points that need to be visited next
        // assign the starting point as the first node int he queue
        struct Queue *q = createQueue();
        enQueue(q, endx, endy);
        int tries = 0;
        bool generation_successful = false;

        // while there are still points in the queue
        while ((!isEmpty(q) || tries < max_tries) && generation_successful == false)
        {
            // dequeue a point from the queue and assign it to the current node
            currnode = deQueue(q);
            clear();
            wprintw(stdscr, "Generating Rivers\n");
            wprintw(stdscr, "Rivers placed = %i, Total Rivers = %i\n", river_count, num_rivers);
            wprintw(stdscr, "River sources checked = %i\n", counter);
            wprintw(stdscr, "Currnode.x = %i, Currnode.y = %i\n", currnode.x, currnode.y);
            wprintw(stdscr, "Tries = %i, Max Tries = %i\n", tries, max_tries);
            refresh();


            // check the point's neighbors
            int dx[8] = { -1, 1, 0, 0, -1, 1, -1, 1 };
            int dy[8] = { 0, 0, -1, 1, -1, 1, 1, -1 };

            for (int i = 0; i < 8; ++i)
            {
                if (generation_successful == true)
                    break;
                // Trying to randomize the array causes seg faults
                //int nx = currnode.x + dx[rand() % (7 - 0 + 1)];
                //int ny = currnode.y + dy[rand() % (7 - 0 + 1)];
                int nx = currnode.x + dx[i];
                int ny = currnode.y + dy[i];

                // If river terminates in water set generation to true and write cells to tile map
                if (tile_map_ptr[ny][nx] == 'w')
                {
                    beginy = currnode.y;
                    beginx = currnode.x;

                    bfs_return = bfs_river(river_map_ptr, noise_map_height, noise_map_width, beginy, beginx, endy, endx);

                    if (bfs_return == 0){
                        river_count++;
                        for (int i = 0; i < noise_map_height; ++i)
                        {
                            for (int j = 0; j < noise_map_width; ++j)
                            {
                                //P for path, s is for the river source, both are given the same tile, though
                                if (river_map_ptr[i][j] == 'p')
                                    tile_map_ptr[i][j] = '~';

                                else if (river_map_ptr[i][j] == 's')
                                    tile_map_ptr[i][j] = 'O';
                            }
                        }
                    }
                    generation_successful = true;
                }
                // Check the neighbor is lower than the current point and not already a river or water,
                // Without the 0.01 I get seg faults
                else if (height_map_ptr[ny][nx] <= height_map_ptr[currnode.y][currnode.x] + 0.01 && river_map_ptr[ny][nx] != 's' && river_map_ptr[ny][nx] != 't' && generation_successful == false)
                {
                    river_map_ptr[ny][nx] = 't';
                    // enQueue next point to check
                    enQueue(q, nx, ny);
                    wprintw(stdscr, "placed path\n");
                }
                else 
                {
                    wprintw(stdscr, "Tries++\n");
                    ++tries;
                    if (tries > max_tries)
                        generation_successful = true;
                }
            }
        }

    }
    free(river_map_ptr);
}


void display_tile_map(WINDOW *right_win, char **tile_map_ptr, COORDINATES *view_port_ptr, int noise_map_height, int noise_map_width)
{
    // Loop through the rows and columns of the tile map
    // TODO: crashes when resizing on edge of map, fix that
    for (int row = view_port_ptr->y; row < view_port_ptr->y + win_dim.rheight - 1; row++)
    {
        for (int col = view_port_ptr->x; col < view_port_ptr->x + win_dim.rwidth - 1; col++)
        {
            char ch = tile_map_ptr[row][col];
            if(ch == 'w' || ch == '~')
            {
                wattron(right_win, COLOR_PAIR(1));
                mvwprintw(right_win, row - view_port_ptr->y, col - view_port_ptr->x, "%c", ch);
                wattroff(right_win, COLOR_PAIR(1));
            }
            else if (ch == '.')
            {
                wattron(right_win, COLOR_PAIR(2));
                mvwprintw(right_win, row - view_port_ptr->y, col - view_port_ptr->x, "%c", ch);
                wattroff(right_win, COLOR_PAIR(2));
            }
            else if (ch == ',' || ch == ';')
            {
                wattron(right_win, COLOR_PAIR(3));
                mvwprintw(right_win, row - view_port_ptr->y, col - view_port_ptr->x, "%c", ch);
                wattroff(right_win, COLOR_PAIR(3));
            }
            else
                mvwprintw(right_win, row - view_port_ptr->y, col - view_port_ptr->x, "%c", ch);
        }
    }
  //  mvwaddch(right_win, pos.y - view_port_ptr->y, pos.x - view_port_ptr->x, '@');

}

void update_terminal_dimensions(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    win_dim.lwidth = cols * .25;
    win_dim.lheight = rows;
    win_dim.rstarty = starty;
    win_dim.rstartx = win_dim.lwidth;
    win_dim.rwidth = cols - win_dim.lwidth;
    win_dim.rheight = rows * .75;
    win_dim.bheight = (rows * .25) + 1;
    win_dim.bwidth = cols * .75;
    win_dim.bstartx = win_dim.lwidth;
    win_dim.bstarty = win_dim.rheight;
}



// Functions for the Perlin Noise map by Nowl on github
int noise2(int x, int y)
{
    int tmp = hash[(y + SEED) % 256];
    return hash[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
    return x + s * (y-x);
}

float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3-2*s));
}

float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int+1, y_int);
    int u = noise2(x_int, y_int+1);
    int v = noise2(x_int+1, y_int+1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}
// Lower freq = More Detail 'zoom'
// Lower depth = less extreme changes 'amp'
float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x*freq;
    float ya = y*freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    //Creates a 'falloff' effect centered around the middle of the height map
    float center_x = 0.5;
    float center_y = 0.5;
    float distance = sqrt((x-center_x)*(x-center_x) + (y-center_y)*(y-center_y));
    float falloff = cos(distance * M_PI);

    int i;
    for(i=0; i<depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return ((fin/div) * falloff);
}

