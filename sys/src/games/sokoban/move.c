#include <u.h>
#include <libc.h>
#include <draw.h>

#include "sokoban.h"

/* pretty ghastly, if you ask me */
void
move(int key)
{
	Point g = level.glenda;
	int moved = 0;

	/* this is messy; no time for math */
	switch(key) {
	case Up:
		switch(level.board[g.x][g.y-1]) {
		case Empty:
		case Goal:
			moved = 1;
			level.glenda = Pt(g.x, g.y-1);
			break;
		case Cargo:
		case GoalCargo:
			switch(level.board[g.x][g.y-2]) {
			case Empty:
				moved = 1;
				level.board[g.x][g.y-2] = Cargo;
				drawboard(Pt(g.x, g.y-2));
				break;
			case Goal:
				moved = 1;
				level.board[g.x][g.y-2] = GoalCargo;
				drawboard(Pt(g.x, g.y-2));
				break;
			}
			if(moved) {
				level.board[g.x][g.y-1] = (level.board[g.x][g.y-1] == Cargo) ? Empty : Goal;
				level.glenda = Pt(g.x, g.y-1);
			}
			break;
		}
		break;
	case Down:
		switch(level.board[g.x][g.y+1]) {
		case Empty:
		case Goal:
			moved = 1;
			level.glenda = Pt(g.x, g.y+1);
			break;
		case Cargo:
		case GoalCargo:
			switch(level.board[g.x][g.y+2]) {
			case Empty:
				moved = 1;
				level.board[g.x][g.y+2] = Cargo;
				drawboard(Pt(g.x, g.y+2));
				break;
			case Goal:
				moved = 1;
				level.board[g.x][g.y+2] = GoalCargo;
				drawboard(Pt(g.x, g.y+2));
				break;
			}
			if(moved) {
				level.board[g.x][g.y+1] = (level.board[g.x][g.y+1] == Cargo) ? Empty : Goal;
				level.glenda = Pt(g.x, g.y+1);
			}
			break;
		}
		break;
	case Left:
		glenda = gleft;
		switch(level.board[g.x-1][g.y]) {
		case Empty:
		case Goal:
			moved = 1;
			level.glenda = Pt(g.x-1, g.y);
			break;
		case Cargo:
		case GoalCargo:
			switch(level.board[g.x-2][g.y]) {
			case Empty:
				moved = 1;
				level.board[g.x-2][g.y] = Cargo;
				drawboard(Pt(g.x-2, g.y));
				break;
			case Goal:
				moved = 1;
				level.board[g.x-2][g.y] = GoalCargo;
				drawboard(Pt(g.x-2, g.y));
				break;
			}
			if(moved) {
				level.board[g.x-1][g.y] = (level.board[g.x-1][g.y] == Cargo) ? Empty : Goal;
				level.glenda = Pt(g.x-1, g.y);
			}
			break;
		}
		break;
	case Right:
		glenda = gright;
		switch(level.board[g.x+1][g.y]) {
		case Empty:
		case Goal:
			moved = 1;
			level.glenda = Pt(g.x+1, g.y);
			break;
		case Cargo:
		case GoalCargo:
			switch(level.board[g.x+2][g.y]) {
			case Empty:
				moved = 1;
				level.board[g.x+2][g.y] = Cargo;
				drawboard(Pt(g.x+2, g.y));
				break;
			case Goal:
				moved = 1;
				level.board[g.x+2][g.y] = GoalCargo;
				drawboard(Pt(g.x+2, g.y));
				break;
			}
			if(moved) {
				level.board[g.x+1][g.y] = (level.board[g.x+1][g.y] == Cargo) ? Empty : Goal;
				level.glenda = Pt(g.x+1, g.y);
			}
			break;
		}
		break;
	}
	if(moved)
		drawboard(Pt(g.x, g.y));

	drawglenda();
}

