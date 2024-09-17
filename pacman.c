/*

Pacman For Console 2.0
By: Alex Helton (alex@opossum-systems.net)
Date: 2024-09-15

LOOK ON MY WORKS, YE MIGHTY, AND DESPAIR!!

*/

// TODO:
/* Update maze graphics to use pipe characters
 * Obviously update sprite graphics to use the new codepoints.
 * Maybe add sound effects
 * Check if I'm digging my own grave
 */

// TO REDUCE PREVENTABLE FRUSTRATION:
// Use the makefile. Do NOT try to gcc it outright!

#define _XOPEN_SOURCE_EXTENDED
#include <stdlib.h>

#include <stdio.h>
#include <ncursesw/ncurses.h>
#include <unistd.h>
#include <string.h>
#include <wchar.h>
#include "pacman.h"
#include <locale.h>

#include <unistd.h>
#include <sys/resource.h>
#include <time.h>

#define EXIT_MSG "Thanks for playing!"
#define END_MSG "Game over..."
#define QUIT_MSG "Bye!"
#define LEVEL_ERR "Cannot find level file: "

void IntroScreen();								  // Show introduction screen and menu
void CheckCollision();							  // See if Pacman and Ghosts collided
void CheckScreenSize();							  // Make sure resolution is at least 32x29
void CreateWindows(int y, int x, int y0, int x0); // Make ncurses windows
void Delay();									  // Slow down game for better control
void DrawWindow();								  // Refresh display
void ExitProgram(const char *message);			  // Exit and display something
void GetInput();								  // Get user input
void InitCurses();								  // Start up ncurses
void LoadLevel(char *levelfile);				  // Load level into memory
void MainLoop();								  // Main program function
void MoveGhosts();								  // Update Ghosts' location
void MovePacman();								  // Update Pacman's location
void PauseGame();								  // Pause

// For ncurses
WINDOW *win;
WINDOW *status;

// For colors
// I should *probably* give this enum a name
enum
{
	Wall = 1,
	Normal,
	Pellet,
	PowerUp,
	GhostWall,
	Ghost1,
	Ghost2,
	Ghost3,
	Ghost4,
	BlueGhost,
	Pacman
};

// I know global variables are bad, but it's just sooo easy!
int Location[5][2] = {0};		// Location of Ghosts and Pacman
int Direction[5][2] = {0};		// Direction of Ghosts and Pacman
int StartingPoints[5][2] = {0}; // Default location in case Pacman/Ghosts die
int Invincible = 0;				// Check for invincibility
int Food = 0;					// Number of pellets left in level
int Level[29][28] = {0};		// Main level array
int LevelNumber = 0;			// What level number are we on?
int GhostsInARow = 0;			// Keep track of how many points to give for eating ghosts
int tleft = 0;					// How long left for invincibility

// defining sleep timer
// compiler gets really mad over using old functions, here's your answer
const struct timespec honkShoo = {0, 1000000L}; // 1 millisecond

int milsleep(long milliseconds)
{
	struct timespec rem;
	struct timespec req = {
		(int)(milliseconds / 1000),		/* seconds, MUST be non-negative */
		(milliseconds % 1000) * 1000000 /* nano, very picky */

	};
	return nanosleep(&req, &rem);
}

int char_to_index(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';  // Map '0'-'9' to 0-9
    }
    if (c >= 'A' && c <= 'G') {
        return c - 'A' + 10;  // Map 'A'-'G' to 10-16
    }
    return -1;  // Invalid character
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	// I was told to do this

	int j = 0;

	InitCurses();
	CheckScreenSize();
	CreateWindows(29, 28, 1, 1);

	// If they specified a level to load
	if ((argc > 1) && (strlen(argv[1]) > 1))
	{
		LoadLevel(argv[1]);
		MainLoop();
	}

	// If not, display intro screen then use default levels
	else
	{
		// Show intro "movie"
		IntroScreen();

		j = 1;
		// They want to start at a level 1-9
		if (argc > 1)
			for (LevelNumber = '1'; LevelNumber <= '9'; LevelNumber++)
				if (LevelNumber == argv[1][0])
					j = LevelNumber - '0';

		// Load 9 levels, 1 by 1, if you can beat all 9 levels in a row, you're awesome
		for (LevelNumber = j; LevelNumber < 10; LevelNumber++)
		{
			LevelFile[strlen(LevelFile) - 6] = '0';
			LevelFile[strlen(LevelFile) - 5] = LevelNumber + '0';
			LoadLevel(LevelFile);
			Invincible = 0; // Reset invincibility
			MainLoop();
		}
	}

	ExitProgram(EXIT_MSG);
}

void CheckCollision()
{
	int a = 0;
	for (a = 0; a < 4; a++)
	{
		// Collision
		if ((Location[a][0] == Location[4][0]) && (Location[a][1] == Location[4][1]))
		{

			// Ghost dies
			if (Invincible == 1)
			{
				Points = Points + GhostsInARow * 20;
				mvwprintw(win, Location[4][0], Location[4][1] - 1, "%d", (GhostsInARow * 20));
				GhostsInARow *= 2;
				wrefresh(win);

				milsleep(500); // Five. Hundred. Milliseconds.

				Location[a][0] = StartingPoints[a][0];
				Location[a][1] = StartingPoints[a][1];
			}

			// Pacman dies
			else
			{
				wattron(win, COLOR_PAIR(Pacman));

				mvwprintw(win, Location[4][0], Location[4][1], "X"); // he's fucking dead

				wrefresh(win);
				milsleep(2500);
				Lives--;

				if (Lives == -1)
				{
					wattron(win, COLOR_PAIR(Ghost1));
					mvwprintw(win, 12, 10, "╔═════════╗");
					mvwprintw(win, 13, 10, "║GAME OVER║");
					mvwprintw(win, 14, 10, "╚═════════╝");
					wrefresh(win);
					// next time, let's make sure this is in its own bracket pair please...
					milsleep(1000);
					ExitProgram(END_MSG);
				}

				// Reset level
				for (a = 0; a < 5; a++)
				{
					Location[a][0] = StartingPoints[a][0];
					Location[a][1] = StartingPoints[a][1];
				}
				Direction[0][0] = 1;
				Direction[0][1] = 0;
				Direction[1][0] = -1;
				Direction[1][1] = 0;
				Direction[2][0] = 0;
				Direction[2][1] = -1;
				Direction[3][0] = 0;
				Direction[3][1] = 1;
				Direction[4][0] = 0;
				Direction[4][1] = -1;

				DrawWindow();

				nanosleep(&honkShoo, NULL);
			}
		}
	}
}

void CheckScreenSize()
{
	// Make sure the window is big enough
	int h, w;
	getmaxyx(stdscr, h, w);
	if ((h < 32) || (w < 29))
	{
		endwin();
		fprintf(stderr, "\nSorry.\n");
		fprintf(stderr, "To play Pacman for Console, your console window must be at least 32x29\n");
		fprintf(stderr, "Please resize your window/resolution and re-run the game.\n\n");
		exit(0);
	}
}

void CreateWindows(int y, int x, int y0, int x0)
{
	win = newwin(y, x, y0, x0);
	status = newwin(3, 27, 29, 1);
}



void Delay()
{
	struct timespec t_start, t_current;
	long elapsed_ms;
	// get start time
	clock_gettime(CLOCK_MONOTONIC, &t_start);

	// slow down the game a little
	do
	{
		GetInput();

		clock_gettime(CLOCK_MONOTONIC, &t_current);

		// calculate difference in millisec
		elapsed_ms = (t_current.tv_sec - t_start.tv_sec) * 1000 + (t_current.tv_nsec - t_start.tv_nsec) / 1000000;

	} while (abs(elapsed_ms) < SpeedOfGame);
}





void DrawWindow()
{
	int draw_Y = 0;
	int draw_X = 0;
	wchar_t chr; // changing to wchar_t allows for f r e a k y unicode support
	int attr;

	// Display level array
	for (draw_Y = 0; draw_Y < 29; draw_Y++)
		for (draw_X = 0; draw_X < 28; draw_X++)
		{
			switch (Level[draw_Y][draw_X])
			{
				// consult the level data files, the numbers correspond to the cases in the loop here
				/*
				0 = "empty space"
				1 = "wall"
				2 = pellet
				3 = power-up
				4 = "ghost wall"

				The "ghost wall" is where ghosts can come in and out of
				but not pacman
				*/

				// I'm *really* afraid to touch this

			case 0:
				chr = L'\u2591';
				attr = A_NORMAL;
				wattron(win, COLOR_PAIR(Normal));
				break;
			case 1:
				chr = L' ';
				attr = A_NORMAL;
				wattron(win, COLOR_PAIR(Wall));
				break;
			case 2:
				chr = '.';
				attr = A_NORMAL;
				wattron(win, COLOR_PAIR(Pellet));
				break;
			case 3:
				chr = L'*';
				attr = A_BOLD;
				wattron(win, COLOR_PAIR(PowerUp));
				break;
			case 4:
				chr = L' ';
				attr = A_NORMAL;
				wattron(win, COLOR_PAIR(GhostWall));
				break;
			}
			// add a single-byte character and rendition to a window and advance the cursor
			// and do that a whole buncha times
			//mvwaddch(win, draw_Y, draw_X, chr | attr); // kinda important, not advisable to remove
			mvwprintw(win, draw_Y, draw_X, "%c", chr);
		}

	int stillAlive;
	// Display number of lives, score, and level
	attr = A_NORMAL;
	wmove(status, 1, 1);
	wattron(status, COLOR_PAIR(Pacman));
	for (stillAlive = 0; stillAlive < Lives; stillAlive++)
		wprintw(status, "C ");
	wprintw(status, "  ");
	wattron(status, COLOR_PAIR(Normal));
	mvwprintw(status, 2, 2, "Level: %d     Score: %d ", LevelNumber, Points);
	wrefresh(status);

	// Display ghosts
	if (Invincible == 0)
	{
		wattron(win, COLOR_PAIR(Ghost1));
		mvwaddch(win, Location[0][0], Location[0][1], '&');
		wattron(win, COLOR_PAIR(Ghost2));
		mvwaddch(win, Location[1][0], Location[1][1], '&');
		wattron(win, COLOR_PAIR(Ghost3));
		mvwaddch(win, Location[2][0], Location[2][1], '&');
		wattron(win, COLOR_PAIR(Ghost4));
		mvwaddch(win, Location[3][0], Location[3][1], '&');
	}

	// OR display vulnerable ghosts
	else
	{
		wattron(win, COLOR_PAIR(BlueGhost));
		mvwaddch(win, Location[0][0], Location[0][1], tleft + '0');
		mvwaddch(win, Location[1][0], Location[1][1], tleft + '0');
		mvwaddch(win, Location[2][0], Location[2][1], tleft + '0');
		mvwaddch(win, Location[3][0], Location[3][1], tleft + '0');
	}

	// Display Pacman
	wattron(win, COLOR_PAIR(Pacman));
	mvwaddch(win, Location[4][0], Location[4][1], 'C');

	wrefresh(win);
}


void ExitProgram(const char *message)
{
	endwin();
	printf("%s\n", message);
	exit(0);
}

void GetInput()
{
	int ch;
	static int chtmp;

	ch = getch();

	// Buffer input
	if (ch == ERR)
		ch = chtmp;
	chtmp = ch;

	switch (ch)
	{
	case KEY_UP:
	case 'w':
	case 'W':
		if ((Level[(Location[4][0] - 1) % 29][Location[4][1]] != 1) && (Level[(Location[4][0] - 1) % 29][Location[4][1]] != 4))
		{
			Direction[4][0] = -1;
			Direction[4][1] = 0;
		}
		break;

	case KEY_DOWN:
	case 's':
	case 'S':
		if ((Level[(Location[4][0] + 1) % 29][Location[4][1]] != 1) && (Level[(Location[4][0] + 1) % 29][Location[4][1]] != 4))
		{
			Direction[4][0] = 1;
			Direction[4][1] = 0;
		}
		break;

	case KEY_LEFT:
	case 'a':
	case 'A':
		if ((Level[Location[4][0]][(Location[4][1] - 1) % 28] != 1) && (Level[Location[4][0]][(Location[4][1] - 1) % 28] != 4))
		{
			Direction[4][0] = 0;
			Direction[4][1] = -1;
		}
		break;

	case KEY_RIGHT:
	case 'd':
	case 'D':
		if ((Level[Location[4][0]][(Location[4][1] + 1) % 28] != 1) && (Level[Location[4][0]][(Location[4][1] + 1) % 28] != 4))
		{
			Direction[4][0] = 0;
			Direction[4][1] = 1;
		}
		break;

	case 'p':
	case 'P':
		PauseGame();
		chtmp = getch();
		break;

	case 'q':
	case 'Q':
		ExitProgram(QUIT_MSG);
		break;
	}
}

void InitCurses()
{
	// The setlocale() must precede initscr

	milsleep(500);
	initscr();
	start_color();
	curs_set(0);
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	nonl();
	cbreak();
	noecho();

	init_pair(Normal, COLOR_WHITE, COLOR_BLACK);
	init_pair(Wall, COLOR_WHITE, COLOR_BLUE);
	init_pair(Pellet, COLOR_WHITE, COLOR_BLACK);
	init_pair(PowerUp, COLOR_BLUE, COLOR_BLACK);
	init_pair(GhostWall, COLOR_WHITE, COLOR_CYAN);
	init_pair(Ghost1, COLOR_RED, COLOR_BLACK);
	init_pair(Ghost2, COLOR_CYAN, COLOR_BLACK);
	init_pair(Ghost3, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(Ghost4, COLOR_YELLOW, COLOR_BLACK);
	init_pair(BlueGhost, COLOR_BLUE, COLOR_RED);
	init_pair(Pacman, COLOR_YELLOW, COLOR_BLACK);
}

void IntroScreen()
{
	int a = 0;
	int b = 23;

	a = getch();
	a = getch();
	a = getch();

	mvwprintw(win, 20, 8, "Press any key...");

	// Scroll Pacman to middle of screen
	for (a = 0; a < 13; a++)
	{
		if (getch() != ERR)
			return;
		wattron(win, COLOR_PAIR(Pacman));
		mvwprintw(win, 8, a, " C");
		wrefresh(win);
		nanosleep(&honkShoo, NULL);
	}

	// Show "Pacman"
	wattron(win, COLOR_PAIR(Pacman));
	mvwprintw(win, 8, 12, "PACMAN");
	wrefresh(win);
	nanosleep(&honkShoo, NULL);

	// Ghosts Chase Pacman
	for (a = 0; a < 23; a++)
	{
		if (getch() != ERR)
			return;
		wattron(win, COLOR_PAIR(Pellet));
		mvwprintw(win, 13, 23, "*");
		wattron(win, COLOR_PAIR(Pacman));
		mvwprintw(win, 13, a, " C");
		wattron(win, COLOR_PAIR(Ghost1));
		mvwprintw(win, 13, a - 3, " &");
		wattron(win, COLOR_PAIR(Ghost3));
		mvwprintw(win, 13, a - 5, " &");
		wattron(win, COLOR_PAIR(Ghost2));
		mvwprintw(win, 13, a - 7, " &");
		wattron(win, COLOR_PAIR(Ghost4));
		mvwprintw(win, 13, a - 9, " &");
		wrefresh(win);
		nanosleep(&honkShoo, NULL);
	}

	nanosleep(&honkShoo, NULL);

	// Pacman Chases Ghosts
	for (a = 25; a > 2; a--)
	{
		if (getch() != ERR)
			return;
		wattron(win, COLOR_PAIR(Pellet));
		mvwprintw(win, 13, 23, " ");

		// Make ghosts half as fast
		if (a % 2)
			b--;

		wattron(win, COLOR_PAIR(BlueGhost));
		mvwprintw(win, 13, b - 9, "& & & &");
		wattron(win, COLOR_PAIR(Pacman));
		mvwprintw(win, 13, b - 9 + 1, " ");
		wattron(win, COLOR_PAIR(Pacman));
		mvwprintw(win, 13, b - 9 + 3, " ");
		wattron(win, COLOR_PAIR(Pacman));
		mvwprintw(win, 13, b - 9 + 5, " ");
		wattron(win, COLOR_PAIR(Pacman));
		mvwprintw(win, 13, b - 9 + 7, " ");
		wattron(win, COLOR_PAIR(Pacman));
		mvwprintw(win, 13, a - 3, "C          ");

		wattron(win, COLOR_PAIR(Pellet));
		mvwprintw(win, 13, 23, " ");
		wrefresh(win);
		nanosleep(&honkShoo, NULL);
	}
}

void LoadLevel(char *levelfile)
{

	int a = 0;
	int b = 0;
	size_t l;
	char error[sizeof(LEVEL_ERR) + 255] = LEVEL_ERR;
	FILE *fin;
	Food = 0;

	// Reset defaults
	Direction[0][0] = 1;
	Direction[0][1] = 0;
	Direction[1][0] = -1;
	Direction[1][1] = 0;
	Direction[2][0] = 0;
	Direction[2][1] = -1;
	Direction[3][0] = 0;
	Direction[3][1] = 1;
	Direction[4][0] = 0;
	Direction[4][1] = -1;

	// Open file
	fin = fopen(levelfile, "r");

	// Make sure it didn't fail
	if (!(fin))
	{
		l = sizeof(error) - strlen(error) - 1;
		strncat(error, levelfile, l);
		if (strlen(levelfile) > l)
			error[sizeof(error) - 2] = '.', error[sizeof(error) - 3] = '.', error[sizeof(error) - 4] = '.';
		ExitProgram(error);
	}

	// Open file and load the level into the array
	for (a = 0; a < 29; a++)
	{
		for (b = 0; b < 28; b++)
		{
			fscanf(fin, "%d", &Level[a][b]);
			if (Level[a][b] == 2)
				Food++;

			if (Level[a][b] == 5)
			{
				Location[0][0] = a;
				Location[0][1] = b;
				Level[a][b] = 0;
			}
			if (Level[a][b] == 6)
			{
				Location[1][0] = a;
				Location[1][1] = b;
				Level[a][b] = 0;
			}
			if (Level[a][b] == 7)
			{
				Location[2][0] = a;
				Location[2][1] = b;
				Level[a][b] = 0;
			}
			if (Level[a][b] == 8)
			{
				Location[3][0] = a;
				Location[3][1] = b;
				Level[a][b] = 0;
			}
			if (Level[a][b] == 9)
			{
				Location[4][0] = a;
				Location[4][1] = b;
				Level[a][b] = 0;
			}
		}
	}

	fscanf(fin, "%d", &LevelNumber);

	// Save initial character points for if Pacman or Ghosts die
	for (a = 0; a < 5; a++)
		StartingPoints[a][0] = Location[a][0], StartingPoints[a][1] = Location[a][1];
}

void MainLoop()
{

	DrawWindow();
	wrefresh(win);
	wrefresh(status);
	nanosleep(&honkShoo, NULL);

	do
	{
		MovePacman();
		//DrawWindow();
		CheckCollision();
		MoveGhosts();
		DrawWindow();
		CheckCollision();
		if (Points > FreeLife)
		{
			Lives++;
			FreeLife *= 2;
		}
		Delay();

	} while (Food > 0);

	DrawWindow();
	nanosleep(&honkShoo, NULL);
	
}

void MoveGhosts()
{
	int a = 0;
	int b = 0;
	int c = 0;
	int tmpx = 0;
	int tmpy = 0;
	int tmpdx = 0;
	int tmpdy = 0;
	int checksides[] = {0, 0, 0, 0, 0, 0};
	static int SlowerGhosts = 0;

	if (Invincible == 1)
	{
		SlowerGhosts++;
		if (SlowerGhosts > HowSlow)
			SlowerGhosts = 0;
	}

	if ((Invincible == 0) || SlowerGhosts < HowSlow)

		// Loop through each ghost
		for (a = 0; a < 4; a++)
		{

			// Switch sides?
			if ((Location[a][0] == 0) && (Direction[a][0] == -1))
				Location[a][0] = 28;
			else if ((Location[a][0] == 28) && (Direction[a][0] == 1))
				Location[a][0] = 0;
			else if ((Location[a][1] == 0) && (Direction[a][1] == -1))
				Location[a][1] = 27;
			else if ((Location[a][1] == 27) && (Direction[a][1] == 1))
				Location[a][1] = 0;
			else
			{

				// Determine which directions we can go
				for (b = 0; b < 4; b++)
					checksides[b] = 0;
				if (Level[Location[a][0] + 1][Location[a][1]] != 1)
					checksides[0] = 1;
				if (Level[Location[a][0] - 1][Location[a][1]] != 1)
					checksides[1] = 1;
				if (Level[Location[a][0]][Location[a][1] + 1] != 1)
					checksides[2] = 1;
				if (Level[Location[a][0]][Location[a][1] - 1] != 1)
					checksides[3] = 1;

				// Don't do 180 unless we have to
				c = 0;
				for (b = 0; b < 4; b++)
					if (checksides[b] == 1)
						c++;

				if (c > 1)
				{
					if (Direction[a][0] == 1)
						checksides[1] = 0;
					else if (Direction[a][0] == -1)
						checksides[0] = 0;
					else if (Direction[a][1] == 1)
						checksides[3] = 0;
					else if (Direction[a][1] == -1)
						checksides[2] = 0;
				}

				c = 0;
				do
				{
					// Decide direction
					b = (int)(rand() / (1625000000 / 4));

					if (checksides[b] == 1)
					{
						if (b == 0)
						{
							Direction[a][0] = 1;
							Direction[a][1] = 0;
						}
						else if (b == 1)
						{
							Direction[a][0] = -1;
							Direction[a][1] = 0;
						}
						else if (b == 2)
						{
							Direction[a][0] = 0;
							Direction[a][1] = 1;
						}
						else if (b == 3)
						{
							Direction[a][0] = 0;
							Direction[a][1] = -1;
						}
					}
					else
					{
						if (Invincible == 0)
						{
							// Chase Pacman
							if ((Location[4][0] > Location[a][0]) && (checksides[0] == 1))
							{
								Direction[a][0] = 1;
								Direction[a][1] = 0;
								c = 1;
							}
							else if ((Location[4][0] < Location[a][0]) && (checksides[1] == 1))
							{
								Direction[a][0] = -1;
								Direction[a][1] = 0;
								c = 1;
							}
							else if ((Location[4][1] > Location[a][1]) && (checksides[2] == 1))
							{
								Direction[a][0] = 0;
								Direction[a][1] = 1;
								c = 1;
							}
							else if ((Location[4][1] < Location[a][1]) && (checksides[3] == 1))
							{
								Direction[a][0] = 0;
								Direction[a][1] = -1;
								c = 1;
							}
						}

						else
						{
							// Run away from Pacman
							if ((Location[4][0] > Location[a][0]) && (checksides[1] == 1))
							{
								Direction[a][0] = -1;
								Direction[a][1] = 0;
								c = 1;
							}
							else if ((Location[4][0] < Location[a][0]) && (checksides[0] == 1))
							{
								Direction[a][0] = 1;
								Direction[a][1] = 0;
								c = 1;
							}
							else if ((Location[4][1] > Location[a][1]) && (checksides[3] == 1))
							{
								Direction[a][0] = 0;
								Direction[a][1] = -1;
								c = 1;
							}
							else if ((Location[4][1] < Location[a][1]) && (checksides[2] == 1))
							{
								Direction[a][0] = 0;
								Direction[a][1] = 1;
								c = 1;
							}
						}
					}

				} while ((checksides[b] == 0) && (c == 0));

				// Move Ghost
				Location[a][0] += Direction[a][0];
				Location[a][1] += Direction[a][1];
			}
		}
}

void MovePacman()
{

	static int itime = 0;

	// Switch sides?
	if ((Location[4][0] == 0) && (Direction[4][0] == -1))
		Location[4][0] = 28;
	else if ((Location[4][0] == 28) && (Direction[4][0] == 1))
		Location[4][0] = 0;
	else if ((Location[4][1] == 0) && (Direction[4][1] == -1))
		Location[4][1] = 27;
	else if ((Location[4][1] == 27) && (Direction[4][1] == 1))
		Location[4][1] = 0;

	// Or
	else
	{
		// Move Pacman
		Location[4][0] += Direction[4][0];
		Location[4][1] += Direction[4][1];

		// If he hit a wall, move back
		if ((Level[Location[4][0]][Location[4][1]] == 1) || (Level[Location[4][0]][Location[4][1]] == 4))
		{
			Location[4][0] -= Direction[4][0];
			Location[4][1] -= Direction[4][1];
		}
	}

	// What is he eating?
	switch (Level[Location[4][0]][Location[4][1]])
	{
	case 2: // Pellet
		Level[Location[4][0]][Location[4][1]] = 0;
		Points++;
		Food--;
		break;
	case 3: // PowerUp
		Level[Location[4][0]][Location[4][1]] = 0;
		Invincible = 1;
		if (GhostsInARow == 0)
			GhostsInARow = 1;
		itime = time(0);
		break;
	}

	// Is he invincible?
	if (Invincible == 1)
		tleft = (11 - LevelNumber - time(0) + itime);

	// Is invincibility up yet?
	if (tleft < 0)
	{
		Invincible = 0;
		GhostsInARow = 0;
		tleft = 0;
	}
}

void PauseGame()
{
	int chtmp;

	// Display pause dialog
	wattron(win, COLOR_PAIR(Pacman));
	mvwprintw(win, 12, 10, "╔══════╗");
	mvwprintw(win, 13, 10, "║PAUSED║");
	mvwprintw(win, 14, 10, "╚══════╝");
	wrefresh(win);

	// And wait
	do
	{
		chtmp = getch();
	} while (chtmp == ERR);
}
