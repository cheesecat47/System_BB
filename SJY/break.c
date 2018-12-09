#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>


#define MAP_WIDTH 62	//except 2 wall lines,  make centre map to  60*20
#define MAP_HEIGHT 22
#define BOARD_HEIGHT MAP_HEIGHT-3	//board moves at map[19][x]
#define BRICK_HEIGHT 10		//brick generated at x : 4~9, y : 7~55


#define WALL 10
#define WALL_BOTTOM 11
#define BRICK1 1
#define BRICK2 2
#define BRICK3 3
#define BOARD 4
#define BALL 5
#define EMPTY 0




int map[MAP_HEIGHT][MAP_WIDTH];
int current_board;
int current_ballX, current_ballY;	
int dx=-1, dy=-1;		//ball delta
int brick_left=0;




// board functions
void setBoard(int c);
void moveBoard(int d);


// ball functions
void setBallPos();
void setBallDel(int what);
void *ballThreadFunc(void*);


//brick functions
void makebrick();
void deleteBrick(int what, int x, int y);


// map functions
void refreshMap();




int main(){
	int h, w; 		//height and width variables for loop	
	int q=1, c=1;	//don't care variables..? not quite importent things
	char ch;


	//to make ball thread
	pthread_t ballThread;
	int thr_id;
	int status;


	/*	Initialise	*/
	initscr();
	clear();		//initialise screen
	curs_set(0);	//make user can't see the cursor
	noecho();
	keypad(stdscr, TRUE);	//enables the reading of func keys(arrow keys)
	

	current_board = MAP_WIDTH/2-4;	//set board at centre
	current_ballX = BOARD_HEIGHT-1;	//set ballX above the board
	current_ballY = MAP_WIDTH/2;		//set ballY at centre

	
	// initialise map using predefined numbers
	for(h=0; h<MAP_HEIGHT; h++){
		for(w=0; w<MAP_WIDTH; w++){
			if( h==0 | w==0 | w==MAP_WIDTH-1 ){
				map[h][w] = WALL;
			}
			else if (h==MAP_HEIGHT-1){
				map[h][w] = WALL_BOTTOM;
			}
			else if(h==BOARD_HEIGHT && (w>=current_board&&w<=current_board+7)){
				map[h][w] = BOARD;
			}
			else
				map[h][w] = EMPTY;
		}
	}
	map[current_ballX][current_ballY] = BALL;
	makebrick();
	refreshMap();


	move(MAP_HEIGHT/2,MAP_WIDTH/2-12);
	addstr("Press any key to start");
	ch=getch();
	move(MAP_HEIGHT/2,MAP_WIDTH/2-12);
	addstr("                         ");


	refreshMap();	//first show map
	thr_id = pthread_create(&ballThread, NULL, ballThreadFunc, (void*)&c);


	while(c){
		c=wgetch(stdscr);	//if user input arrow key
		setBoard(c);		//handle arrow key
		refreshMap();		//loop continue
	}
	

	pthread_join(ballThread, (void**)&status);


	echo();
	getch();
	endwin();	//end curses
	return 0;
}




void refreshMap(){
	int h, w;


	for(h=0; h<MAP_HEIGHT; h++){
		for(w=0; w<MAP_WIDTH; w++){
			move(h,w);
			if(map[h][w]==EMPTY){
				addch(' ');
			}
			else if(map[h][w]==WALL | map[h][w]==WALL_BOTTOM){
				addch('#');
			}
			else if(map[h][w]==BOARD){
				addch('W');
			}
			else if(map[h][w]==BALL){
				addch('O');
			}
			else if(map[h][w]==BRICK3){
				addch('C');
			}
			else if(map[h][w]==BRICK2) {
				addch('B');
			}
			else if(map[h][w]==BRICK1) {
				addch('A');
			}
		}
	}
	refresh();
}




void setBoard(int c){
	int i, sth;


	switch(c){
		case KEY_LEFT:
			sth = map[BOARD_HEIGHT][current_board-1];	//check left
			if(sth==0){		//empty left
				moveBoard(-1);
			}else if(sth==1){;}// there is wall, don't move
			break;
		case KEY_RIGHT:
			sth = map[BOARD_HEIGHT][current_board+8];	//check right
			if(sth==0){
				moveBoard(1);
			}else if(sth==1){;}
			break;
	}
}


void moveBoard(int d){
	int i;


	if(d==-1){
		map[BOARD_HEIGHT][current_board+7]=0;	//set rightend to empty
		map[BOARD_HEIGHT][--current_board]=3;	//set leftend to board
	}else if(d==1){
		map[BOARD_HEIGHT][current_board]=0;
		map[BOARD_HEIGHT][++current_board+7]=3;
	}
}



void *ballThreadFunc(void* data){
	while(1){
		setBallPos();
	}
}


void setBallPos(){
	int temp;


	temp = map[current_ballX+dx][current_ballY+dy];
	switch(temp){
		case WALL:	//when next pos is wall, bounce
			setBallDel(0);
			break;
		case WALL_BOTTOM:	//when next pos is bottom, game end
			echo();
			endwin();
			exit(1);
			break;
		case BOARD:		//when next pos is board, bounce
			setBallDel(0);
			break;
		case BRICK3:		//when next pos is brick, break the brick and bounce
		case BRICK2:
		case BRICK1:
			setBallDel(temp);
			break;
	}


	//make prev ball to empty
	map[current_ballX][current_ballY]=EMPTY;
	//set new position
	current_ballX+=dx;
	current_ballY+=dy;
	map[current_ballX][current_ballY]=BALL;


	refreshMap();
	usleep(200000);	
}


void setBallDel(int what){
	if(map[current_ballX][current_ballY-1]!=EMPTY){
		//if left is not empty
		dy=1;
		deleteBrick(what, 0,-1);
	}else if(map[current_ballX][current_ballY+1]!=EMPTY){
		//if right is not empty
		dy=-1;
		deleteBrick(what, 0, 1);
	}else if(map[current_ballX-1][current_ballY]!=EMPTY){
		//if up is not empty
		dx=1;
		deleteBrick(what, -1, 0);
	}else if(map[current_ballX+1][current_ballY]!=EMPTY){
		//if down is not empty
		dx=-1;
		deleteBrick(what, 1, 0);
	}else if(map[current_ballX-1][current_ballY-1]!=EMPTY){
		//if top left corner is not empty
		//this may be work if ball collide at the corner of bricks or board
		dx=1; dy=1;
		deleteBrick(what, -1, -1);
	}else if(map[current_ballX-1][current_ballY+1]!=EMPTY){
		//if top right corner is not empty
		dx=1; dy=-1;
		deleteBrick(what, -1, 1);
	}else if(map[current_ballX+1][current_ballY-1]!=EMPTY){
		//if bottom left corner is not empty
		dx=-1; dy=1;
		deleteBrick(what, 1, -1);
	}else if(map[current_ballX+1][current_ballY+1]!=EMPTY){
		//if bottom right corner is not empty
		dx=-1; dy=-1;
		deleteBrick(what, 1, 1);
	}
}




void makebrick(){
	int i, j;


	for(i=4; i<BRICK_HEIGHT; i+=2){
		for(j=7; j<MAP_WIDTH-7; j++){
			map[i][j]=4;
			brick_left+=2;
			if(j%4==1) j++;
		}
	}
}


void deleteBrick(int what, int x, int y){
	int xpos = current_ballX+x, ypos=current_ballY+y;
	int temp=ypos;

	if(what==BRICK3 || what==BRICK2||what==BRICK1){
		map[xpos][ypos]--;
		while(map[xpos][--temp]!=EMPTY){
			map[xpos][temp]--;
		}
		temp=ypos;
		while(map[xpos][++temp]!=EMPTY){
			map[xpos][temp]--;
		}
		brick_left--;
	}
}
