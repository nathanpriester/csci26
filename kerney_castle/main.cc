#include <tuple>
#include <deque>
#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <ncurses.h>
#include <utility>
#include <limits>
#include "splash.h"
using namespace std;



const unsigned char OPEN = '.'; //An open space
const unsigned char WALL = '#'; //A wall space; impassable
const unsigned char KING = 'K'; //Represents the King
const unsigned char PAWN = 'p'; //Represents the enemy pawns trying to capture the King
const unsigned char TREASURE = '$'; //Represents money, which the pawns will move to capture and move off screen
const unsigned char PAWN_WITH_TREASURE = 'P'; //A pawn carrying money off the screen - if it touches an edge it drops off the gold and becomes a normal pawn again

//Globals holding game state
int SIZE_X,SIZE_Y; //Dimensions of the castle
int cursor_x, cursor_y; //Cursor for drawing stuff
unsigned int max_kings, king_count; //Holds number of kings in the world
unsigned char* castle;
unsigned int max_pawns, pawn_count; //Holds number of pawns in the world
bool game_on; //Holds if the simulation is running
//vector<int> king_coords;
//vector<int> treasure_coords;
//vector<int> wall_coords;
//vector<int> pawn_coords;

const unsigned int MIN_SIZE = 8; //Minimum size of the castle
const unsigned int TIMEOUT = 300; //Milliseconds to wait for a getch to finish
const int UP = 65; //Key code for up arrow
const int DOWN = 66;
const int LEFT = 68;
const int RIGHT = 67;

//2D to 1D array mapping
//NOTE: creates a circular array
//For example, access the castle location (3,2) like this:
//  castle[index(3,2)]
int index(int i, int j) {
	while (i < 0) i+=SIZE_X;
	while (j < 0) j+=SIZE_Y;
	if (i >= SIZE_X) i %= SIZE_X;
	if (j >= SIZE_Y) j %= SIZE_Y;
	return (i*SIZE_Y+j);
}

void reset_castle(unsigned char value = OPEN) {
	for (int i = 0; i < SIZE_X; i++)
		for (int j = 0; j < SIZE_Y; j++)
			if (i == 0 || j == 0 || i == SIZE_X - 1 || j == SIZE_Y - 1) //Is edge
				castle[index(i,j)] = WALL;
			else
				castle[index(i,j)] = OPEN;
}

//Prints walls surrounding the castle, and then the contents of castle inside it
void print_castle() {
	for (int i = 0; i < SIZE_X; i++) {
		for (int j = 0; j < SIZE_Y; j++) {
			if (i == cursor_x && j == cursor_y)
				attron(A_UNDERLINE | A_BOLD);
			//if (i == 0 || j == 0 || i == SIZE_X - 1 || j == SIZE_Y - 1) //Is edge
			//	mvaddch(i,j,WALL);
			//else  
			mvaddch(i,j,castle[index(i,j)]);
			attroff(A_UNDERLINE | A_BOLD);
		}
	}
}

void die() {
	cout << "Invalid input.\n";
	exit(EXIT_FAILURE);
}


void wait_ticks(int ticks) {
	clock_t start = clock();
	while ((clock() - start) < ticks)
		; //Do nothing
}

//YOU: You write this code. It will control the pawns, and will be called every frame the game is running
//The rules for pawns are as follows:
//1. They must move one square each frame, diagonals included
// 1.1. If they can move toward their nearest target (see #3 below) they will do so
// 1.2. If they cannot move toward their nearest target they will move randomly if they can
// 1.3. If they are completely blocked in, they do not move at all
//2. They cannot pass through other pawns or walls.
// (i.e. They can only move through open squares or into KING squares to capture them)
//3. Every frame, they calculate their closest target, which is the nearest (shortest path)
// TREASURE or KING square. 
// 3.1. If they are adjacent to a KING, that takes precedence over all else, and will capture the KING
//  Capturing a king will decrement the king_count.
// 3.2. Otherwise, if there is a tie (equal distance, greater than 1) between a TREASURE and a KING they
//  will choose to move to the TREASURE over the KING
//4. If there are no treasures or Kings and there are Pawns, the Pawns win, and print "Game Over Pawns Win"
//5. If a pawn moves over a TREASURE, it becomes a PAWN_WITH_TREASURE, which changes how it moves:
// 5.1. Pawns with treasure ignore kings entirely (and will refuse to capture them, and path around them)
// 5.2. Pawns with treasure will path to the nearest edge, moving one square each round
// 5.3. If a Pawn reaches an edge, it will drop off the treasure, becoming a normal PAWN again

typedef tuple<int,int> Point;
Point sentinel = make_tuple(-999,-999);

vector<Point> neighbors(Point input, vector<Point> came_from, vector<double>& cost_so_far){
	vector<Point> temp;
	int x = get<0>(input);
	int y = get<1>(input);
	double weight;
	for(int i=-1; i<2; i++){
		for(int j=1; j>-2; j--){
			if(i==0 && j==0) continue;
			if(i==0 || j==0) weight = cost_so_far[index(x,y)] + 1;
			else weight = cost_so_far[index(x,y)] + 1.4;
			unsigned char tmp = castle[index(x+i,y+j)];
			if (came_from[index(x+i,y+j)] != sentinel && weight < cost_so_far[index(x+i,y+j)]){
				temp.push_back(make_tuple(x+i,y+j));
				cost_so_far[index(x+i,y+j)] = weight;
			}
			else if((tmp==OPEN || tmp==KING || tmp==TREASURE) && came_from[index(x+i,y+j)] == sentinel){
				temp.push_back(make_tuple(x+i,y+j));
				cost_so_far[index(x+i,y+j)] = weight;
			}
		}
	}
	return temp;
}

vector<Point> neighbors_edges(Point input, vector<Point> came_from, vector<double>& cost_so_far){
	vector<Point> temp;
	int x = get<0>(input);
	int y = get<1>(input);
	double weight;
	for(int i=-1; i<2; i++){
		for(int j=1; j>-2; j--){
			if(i==0 && j==0) continue;
			if(i==0 || j==0) weight = cost_so_far[index(x,y)] + 1;
			else weight = cost_so_far[index(x,y)] + 1.4;
			unsigned char tmp = castle[index(x+i,y+j)];
			if (came_from[index(x+i,y+j)] != sentinel && weight < cost_so_far[index(x+i,y+j)]){
				temp.push_back(make_tuple(x+i,y+j));
				cost_so_far[index(x+i,y+j)] = weight;
			}
			else if((tmp==OPEN || x+i == 0 || y+j == 0 || x+i == SIZE_X - 1 || y+j == SIZE_Y - 1) && came_from[index(x+i,y+j)] == sentinel){
				temp.push_back(make_tuple(x+i,y+j));
				cost_so_far[index(x+i,y+j)] = weight;
			}
		}
	}
	return temp;
}

int count = 1;
void ai_pawn(int i, int j) { //i and j are the coordinates the pawn is at
	//cout<<" count: "<<count++<<" ";

	Point start = make_tuple(i,j);
	Point goal = sentinel;

	vector<Point> came_from(SIZE_X*SIZE_Y,sentinel);
	vector<double> cost_so_far(SIZE_X*SIZE_Y,0);
	deque<Point> frontier;

	frontier.push_back(start);

	bool treasure_bool = false;
	if(castle[index(i,j)] == PAWN_WITH_TREASURE) treasure_bool = true;

	while(!frontier.empty()){
		auto current = frontier.front();
		frontier.pop_front();
		int curr_x = get<0>(current);
		int curr_y = get<1>(current);
		unsigned char test = castle[index(curr_x,curr_y)];

		if((test == KING || test == TREASURE) && !treasure_bool){
			goal = make_tuple(curr_x,curr_y);
			break;
		}
		if((curr_x == 0 || curr_y == 0 || curr_x == SIZE_X - 1 || curr_y == SIZE_Y - 1) && treasure_bool){
			goal = make_tuple(curr_x,curr_y);
			break;
		}
		if(!treasure_bool){
			for(auto next : neighbors(current,came_from,cost_so_far)){
				int next_x = get<0>(next);
				int next_y = get<1>(next);
				frontier.push_back(next);
				came_from[index(next_x,next_y)] = current;
			}
		}
		else
			for(auto next : neighbors_edges(current,came_from,cost_so_far)){
				int next_x = get<0>(next);
				int next_y = get<1>(next);
				frontier.push_back(next);
				came_from[index(next_x,next_y)] = current;
			}
	}
	
	//cout<<"I: "<<i<<" J: "<<j<<"  ";
	//cout<<"goal: "<<get<0>(goal)<<get<1>(goal)<<"   ";
	//cout<<"start: "<<get<0>(start)<<get<1>(start)<"   ";
	//Point testing = came_from[index(5,6)];
	//cout<<"testing: "<<get<0>(testing)<<get<1>(testing)<<"  ";
	for(int x=-1; x<2; x++){
		for(int y=1; y>-2; y--){
			if(x==0 && y==0) continue;
			if(castle[index(x+i,y+j)] == KING){
				castle[index(i,j)] == OPEN;
				castle[index(x+i,y+j)] == PAWN;
				king_count--;
				return;
			}
		}
	}

	if(goal != sentinel){
		Point pathfind = goal;
		while(true){
			int pathfind_x = get<0>(pathfind);
			int pathfind_y = get<1>(pathfind);
			Point temp = came_from[index(pathfind_x,pathfind_y)];
			if(temp == start) break;
			pathfind = temp;
		}

		//cout<<" loop: "<<get<0>(loop)<<get<1>(loop)<<" ";
		//
		//if(castle[index(get<0>(pathfind),get<1>(pathfind))] == KING) king_count--;
		if(castle[index(get<0>(pathfind),get<1>(pathfind))] == TREASURE){
			cout<<"asdkfjdsakj";
			castle[index(i,j)] = OPEN;
			castle[index(get<0>(pathfind),get<1>(pathfind))] = PAWN_WITH_TREASURE;
			return;
		}
		else if (castle[index(i,j)] == PAWN_WITH_TREASURE){
			castle[index(i,j)] = OPEN;
			castle[index(get<0>(pathfind),get<1>(pathfind))] = PAWN_WITH_TREASURE;
			return;
		}
		else{
			castle[index(i,j)] = OPEN;
			castle[index(get<0>(pathfind),get<1>(pathfind))] = PAWN;
			return;
		}
	}
	else{
		//behavior if no path to king or treasure exists
	}

	/*
	   int new_x = i + (rand() % 3 - 1); //Move randomly in x direction
	   int new_y = j + (rand() % 3 - 1); //Move randomly in y direction
	   if (castle[index(new_x,new_y)] == KING) { //Woot
	   castle[index(i,j)] = OPEN; //Vacate current spot
	   castle[index(new_x,new_y)] = PAWN; //Put us in new spot
	   king_count--;
	   }
	   else if (castle[index(new_x,new_y)] == OPEN) { 
	   castle[index(i,j)] = OPEN;
	   castle[index(new_x,new_y)] = PAWN;
	   }
	   */
	return;
}
//The AI for Kings is simple. If there is a pawn next to us, we move on top of them and capture them
// This removes them from the game, and decrements pawn_counter.
//If there are at least one kings in the game and no pawns, then print a "Game Over Kings Win" message
void ai_king(int i, int j) {
	for(int x = -1; x < 2; x++){
		for(int y = -1; y < 2; y++){
			int current = index(x+i,y+j);
			if(castle[current] == PAWN){
				castle[index(i,j)] = OPEN;
				castle[current] = KING;
				pawn_count--;
				return;
			}
		}
	}
}

int main()
{
	cout << "How big a castle do you want? (Enter x size followed by y then hit return): ";
	cin >> SIZE_Y >> SIZE_X; //Everything is backwards (x and y) in NCURSES
	if (!cin || SIZE_X < MIN_SIZE || SIZE_Y < MIN_SIZE) {
		cout << "Error: castle size must be at least " << MIN_SIZE << " in each dimension.\n";
		return 1;
	}
	castle = new unsigned char[SIZE_X * SIZE_Y];
	reset_castle(OPEN);

	//Set up NCURSES
	initscr();//Start curses mode
	clear();
	noecho();
	cbreak();
	timeout(TIMEOUT); //Set a max delay for key entry
	
	splash();
	reset();

	//MAIN LOOP
	int frame = 0;
	cursor_x = SIZE_X/2, cursor_y = SIZE_Y/2; //Cursor for drawing stuff
	game_on = false;
	while (true) {
		if (!game_on) 
			mvprintw(SIZE_X,0,"Design Mode - Game Paused                                        ");
		else
			mvprintw(SIZE_X,0,"Game Running. Kings: %i Pawns: %i Frame: %i",king_count, pawn_count, frame++);
		mvprintw(SIZE_X+1,0,"Use arrow keys to move the cursor around");
		mvprintw(SIZE_X+2,0,"Type Space to clear the square or set a wall");
		mvprintw(SIZE_X+3,0,"Type K to place a King");
		mvprintw(SIZE_X+4,0,"Type P to place a Pawn");
		mvprintw(SIZE_X+5,0,"Type $ to place a Treasure");
		mvprintw(SIZE_X+6,0,"Type Enter to run or pause the game");
		mvprintw(SIZE_X+7,0,"Type Q to quit");
		int ch = getch(); // Wait for user input, with TIMEOUT delay
		if (ch == 'q' || ch == 'Q') break;
		else if (ch == ' ' || ch == '#') {
			if (castle[index(cursor_x,cursor_y)] == OPEN)
				castle[index(cursor_x,cursor_y)] = WALL;
			//wall_coords.puck_back(cursor_x);
			//wall_coords.puck_back(cursor_y);
			else
				castle[index(cursor_x,cursor_y)] = OPEN;
		}
		else if (ch == 'K' || ch == 'k') {
			if (castle[index(cursor_x,cursor_y)] == OPEN) {
				castle[index(cursor_x,cursor_y)] = KING;
				//king_coords.puck_back(cursor_x);
				//king_coords.puck_back(cursor_y);
				king_count++;
				max_kings++;
			}
		}
		else if (ch == 'p' || ch == 'P') {
			if (castle[index(cursor_x,cursor_y)] == OPEN) {
				castle[index(cursor_x,cursor_y)] = PAWN;
				//pawn_coords.puck_back(cursor_x);
				//pawn_coords.puck_back(cursor_y);
				pawn_count++;
				max_pawns++;
			}
		}
		else if (ch == '4' || ch == '$') {
			if (castle[index(cursor_x,cursor_y)] == OPEN) {
				castle[index(cursor_x,cursor_y)] = TREASURE;
				//treasure_coords.puck_back(cursor_x);
				//treasure_coords.puck_back(cursor_y);
			}
		}
		//Handle arrow keys, the NCURSES #defs don't work here for some reason
		//KEY_UP, for example, is set to 0403 (259), not 65
		else if (ch == RIGHT) { 
			cursor_y++;
			if (cursor_y >= SIZE_Y) cursor_y = SIZE_Y - 1; //Clamp value
		}
		else if (ch == LEFT) {
			cursor_y--;
			if (cursor_y < 0) cursor_y = 0; 
		}
		else if (ch == UP) { //Should be KEY_UP, grr
			cursor_x--;
			if (cursor_x < 0) cursor_x = 0;
		}
		else if (ch == DOWN) {
			cursor_x++;
			if (cursor_x >= SIZE_X) cursor_x = SIZE_X - 1; //Clamp value
		}
		else if (ch == ERR) { //No keystroke
			; //Do nothing
		} 
		else if (ch == '\n') {
			game_on = !game_on; //Pause or unpause the game
		}
		//else
		//mvprintw(SIZE_Y+5,0,"Unknown character: %i\n",ch);

		//Run the AI movement for pawns and kings
		if (game_on) {
			for (int i = 0; i < SIZE_X; i++) {
				for (int j = 0; j < SIZE_Y; j++) {
					if (castle[index(i,j)] == PAWN || castle[index(i,j)] == PAWN_WITH_TREASURE){
						print_castle();
						ai_pawn(i,j);//Run the AI on all pawns in the castle
						wait_ticks(20000);
					}
					else if (castle[index(i,j)] == KING) {
						//print_castle();
						ai_king(i,j); //Run the AI on all kings in the castle
						//refresh();
						//wait_ticks(20000);
					}
				}
			}
		}

		//Check for victory -
		// At least one king and pawn was placed in the game, and now there's none
		if (max_kings > 0 && max_pawns > 0 && (king_count <= 0 || pawn_count <= 0)) {
			clear();
			break;
		}

		print_castle();
		refresh(); // Print it on to the real screen
		//wait_ticks(300000); //Additional pause if you want
	}
	endwin(); // End curses mode

	//You could delete this and put in a better win screen in CURSES
	system("clear");
	cout << "GAME OVER: ";
	if (pawn_count <= 0) {
		cout << "The Kings Win!\n";
	}
	if (king_count <= 0) {
		cout << "The Pawns Win!\n";
	}

	return 0;
}
