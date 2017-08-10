// TankWarfare.cpp : Defines the entry point for the console application.
//

#define _WIN32_WINNT 0x0500
#include <windows.h>

#include <fstream>
#include <iostream>
#include <string>
#include <conio.h>
#include <stdlib.h>
#include <time.h>
#include <list>

const int WORLD_HEIGHT = 80;
const int WORLD_WIDTH = 200;

const int VIEWPORT_HEIGHT = 21;
const int VIEWPORT_WIDTH = 51;

const int ARROW_UP_KEY = 72;
const int ARROW_DOWN_KEY = 80;
const int ARROW_LEFT_KEY = 75;
const int ARROW_RIGHT_KEY = 77;

const int PLAYER_SHOOT_KEY = 32;

const int MAX_BULLET_LIMIT = 10;
const int BULLET_MAX_LIFETIME = 20;

const char* PLAYER_CHARACTER = "@";
const char* ENEMY_CHARACTER = "&";
const char* BULLET_CHARACTER = "*";
const char* WALL_CHARACTER = "#";
const char* EMPTY_CHARACTER = " ";

const int WORLD_MOVEMENT_SPEED = 1;
const int PLAYER_MOVEMENT_SPEED = 1;
const int ENEMY_MOVEMENT_SPEED = 1;
const int BULLET_MOVEMENT_SPEED = 1;

const int FRAMERATE = 10;
const double FRAMETIME = 1000/FRAMERATE;

const enum Direction {
	UP = 0,
	DOWN = 1,
	LEFT = 2,
	RIGHT = 3
};

const enum UnitType {
	PLAYER_TYPE = 0,
	ENEMY_TYPE = 1,
	BULLET_TYPE = 2
};

class Unit {
public:
	 COORD position;
	 const char* unitCharacter;
	 Direction dir;
	 UnitType type;
	 int lifeTime;
	 std::list<Unit*> bullets;
	
	 Unit(COORD pos, UnitType unitType, const char* unitchar) {
		 position.Y = pos.Y;
		 position.X = pos.X;
		 unitCharacter = unitchar;
		 dir = UP;
		 type = unitType;
		 lifeTime = 0;
	}
	
};

class World
{

private:
	HANDLE hStdout;
	SMALL_RECT srctReadRect;
	SMALL_RECT srctWriteRect;
	COORD coordBufSize;
	BOOL fSuccess;
	
	CHAR_INFO worldArray[WORLD_HEIGHT*WORLD_WIDTH];
public:
	COORD coordBufCoord;
	Unit* player;
	std::list<Unit*> enemies;
	bool gameOver;
	bool winner;
	World() {
		gameOver = false;
		winner = false;
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdout == INVALID_HANDLE_VALUE)
		{
			printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
			return;
		}
	}

	void InitializeFromFile() {
		std::ifstream file("../map.txt");
		std::string data;
		int i = 0;
		if (file.is_open()) {
			while (getline(file, data)) {
				for (int q = 0; q < data.length(); q++) {
					worldArray[(i*WORLD_WIDTH) + q].Char.UnicodeChar = data[q];
					worldArray[(i*WORLD_WIDTH) + q].Attributes = 0x0040;
					COORD tmpCoord;
					tmpCoord.Y = i;
					tmpCoord.X = q;
					if (data[q] == *PLAYER_CHARACTER) {
						player = new Unit(tmpCoord, PLAYER_TYPE, PLAYER_CHARACTER);
					} else if(data[q] == *ENEMY_CHARACTER) {
						Unit* newEnemy = new Unit(tmpCoord, ENEMY_TYPE, ENEMY_CHARACTER);
						enemies.push_back(newEnemy);
					}
				}
				i++;
			}

			file.close();
		}
		else {
			std::cout << "File was not find and didn't open\n";
		}

	}

	bool HitWall(COORD newPosition) {
		return worldArray[(newPosition.Y*WORLD_WIDTH) + newPosition.X].Char.UnicodeChar == *WALL_CHARACTER;
	}

	bool HitPlayer(COORD newPosition) {
		return worldArray[(newPosition.Y*WORLD_WIDTH) + newPosition.X].Char.UnicodeChar == *PLAYER_CHARACTER;
	}

	bool HitEnemy(COORD newPosition) {
		return worldArray[(newPosition.Y*WORLD_WIDTH) + newPosition.X].Char.UnicodeChar == *ENEMY_CHARACTER;
	}

	bool HitBullet(COORD newPosition) {
		return worldArray[(newPosition.Y*WORLD_WIDTH) + newPosition.X].Char.UnicodeChar == *BULLET_CHARACTER;
	}

	void TryMoveUnit(Unit* unit, COORD newPosition) {
		char newPositionChar = worldArray[(newPosition.Y*WORLD_WIDTH) + newPosition.X].Char.UnicodeChar;
		switch (unit->type) {
		case PLAYER_TYPE:
			if (!HitWall(newPosition) && !HitBullet(newPosition)) {
				if (HitEnemy(newPosition)) {
					gameOver = true;
					winner = false;
				} else {
					MoveUnit(unit, newPosition);
				}
			}
			break;
		case ENEMY_TYPE:
			if (!HitWall(newPosition) && !HitEnemy(newPosition) && !HitBullet(newPosition)) {
				if (HitPlayer(newPosition)) {
					gameOver = true;
					winner = false;
				} else {
					MoveUnit(unit, newPosition);
				}
			}
			break;
		case BULLET_TYPE:
			if (!HitWall(newPosition) && !HitBullet(newPosition) && !HitPlayer(newPosition)) {
				if (HitEnemy(newPosition)) {
					KillEnemy(newPosition);
				} else {
					MoveUnit(unit, newPosition);
				}
			}
			break;
		default:
			break;
		}
	}

	void KillEnemy(COORD position) {
		std::list<Unit*>::iterator enemyIt;
		for (enemyIt = enemies.begin(); enemyIt != enemies.end();) {
			Unit *currentEnemy = dynamic_cast<Unit*>(*enemyIt);
			COORD currentEnemyCoord = currentEnemy->position;
			if (currentEnemyCoord.Y == position.Y && currentEnemyCoord.X == position.X) {
				EraseUnit(currentEnemy);
				enemyIt = enemies.erase(enemyIt);
			}
			else {
				++enemyIt;
			}
		}
	}

	void MoveUnit(Unit* unit, COORD newPosition) {
		worldArray[(unit->position.Y*WORLD_WIDTH) + unit->position.X].Char.UnicodeChar = *EMPTY_CHARACTER;
		worldArray[(newPosition.Y*WORLD_WIDTH) + newPosition.X].Char.UnicodeChar = *unit->unitCharacter;
		unit->position.Y = newPosition.Y;
		unit->position.X = newPosition.X;
	}

	bool DrawNewUnit(Unit* unit) {
		if (!HitWall(unit->position)) {
			worldArray[(unit->position.Y*WORLD_WIDTH) + unit->position.X].Char.UnicodeChar = *unit->unitCharacter;
			return true;
		}
		return false;
	}

	void EraseUnit(Unit* unit) {
		worldArray[(unit->position.Y*WORLD_WIDTH) + unit->position.X].Char.UnicodeChar = *EMPTY_CHARACTER;
	}

	void RenderMap(int topLeftX, int topLeftY) {
		// Set the destination rectangle.
		srctWriteRect.Top = 0;    // top left: row 0, col 0 
		srctWriteRect.Left = 0;
		srctWriteRect.Bottom = VIEWPORT_HEIGHT - 1; // bot. right: row 79, col 199 
		srctWriteRect.Right = VIEWPORT_WIDTH - 1;

		// The temporary buffer size is 80 rows x 200 columns. 

		coordBufSize.Y = WORLD_HEIGHT;
		coordBufSize.X = WORLD_WIDTH;

		// The top left destination cell of the temporary buffer is 
		// row 0, col 0. 

		coordBufCoord.Y = topLeftY;
		coordBufCoord.X = topLeftX;

		fSuccess = WriteConsoleOutput(
			hStdout, // screen buffer to write to 
			worldArray,        // buffer to copy from 
			coordBufSize,     // col-row size of chiBuffer 
			coordBufCoord,    // top left src cell in chiBuffer 
			&srctWriteRect);  // dest. screen buffer rectangle 
		if (!fSuccess)
		{
			printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
			return;
		}
	}

};

World *tankMap;

void AdaptConsole(int Height, int Width) {
	_COORD coord;
	coord.Y = Height;
	coord.X = Width;

	_SMALL_RECT Rect;
	Rect.Top = 0;
	Rect.Left = 0;
	Rect.Bottom = Height - 1;
	Rect.Right = Width - 1;

	HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);      // Get Handle
	SetConsoleScreenBufferSize(Handle, coord);            // Set Buffer Size 
	SetConsoleWindowInfo(Handle, TRUE, &Rect);            // Set Window Size 
}
void StartGameLoop() {
	unsigned char ch;
	do {
		
		clock_t startTime, endTime;
		startTime = clock();

		COORD currentWorldCoord = tankMap->coordBufCoord;
		COORD newWorldCoord = currentWorldCoord;

		COORD currentPlayerCoord = tankMap->player->position;
		COORD newPlayerCoord = currentPlayerCoord;
		int playerBulletCount = tankMap->player->bullets.size();
		COORD newBulletCoord = currentPlayerCoord;

		int enemyCount = tankMap->enemies.size();
		if (enemyCount <= 0){
			tankMap->gameOver = true;
			tankMap->winner = true;
		}
		if (_kbhit()) {
			ch = _getch();
			switch (ch)
			{
			case PLAYER_SHOOT_KEY:
				if (playerBulletCount <= MAX_BULLET_LIMIT) {
					switch (tankMap->player->dir) {
					case UP:
						if (!(currentPlayerCoord.Y - PLAYER_MOVEMENT_SPEED < 0)) {
							newBulletCoord.Y -= BULLET_MOVEMENT_SPEED;
							Unit* newBullet = new Unit(newBulletCoord, BULLET_TYPE, BULLET_CHARACTER);
							newBullet->dir = UP;
							if (tankMap->DrawNewUnit(newBullet))
								tankMap->player->bullets.push_back(newBullet);
						}
						break;
					case DOWN:
						if (!(currentPlayerCoord.Y + PLAYER_MOVEMENT_SPEED > WORLD_HEIGHT)) {
							newBulletCoord.Y += BULLET_MOVEMENT_SPEED;
							Unit* newBullet = new Unit(newBulletCoord, BULLET_TYPE, BULLET_CHARACTER);
							newBullet->dir = DOWN;
							if (tankMap->DrawNewUnit(newBullet))
								tankMap->player->bullets.push_back(newBullet);
						}
						break;
					case LEFT:
						if (!(currentPlayerCoord.X - PLAYER_MOVEMENT_SPEED < 0)) {
							newBulletCoord.X -= BULLET_MOVEMENT_SPEED;
							Unit* newBullet = new Unit(newBulletCoord, BULLET_TYPE, BULLET_CHARACTER);
							newBullet->dir = LEFT;
							if (tankMap->DrawNewUnit(newBullet))
								tankMap->player->bullets.push_back(newBullet);
						}
						break;
					case RIGHT:
						if (!(currentPlayerCoord.X + PLAYER_MOVEMENT_SPEED > WORLD_WIDTH)) {
							newBulletCoord.X += BULLET_MOVEMENT_SPEED;
							Unit* newBullet = new Unit(newBulletCoord, BULLET_TYPE, BULLET_CHARACTER);
							newBullet->dir = RIGHT;
							if (tankMap->DrawNewUnit(newBullet))
								tankMap->player->bullets.push_back(newBullet);
						}
						break;
					default:
						break;
					}
				}
				break;
			case 27:// press ESC to exit
				tankMap->gameOver = true;
				tankMap->winner = false;
				break;
			case 0:
			case 224:
				ch = _getch();
				switch (ch)
				{
				case ARROW_UP_KEY:
					/* Code for up arrow handling */
					if (!(currentPlayerCoord.Y - PLAYER_MOVEMENT_SPEED < 0)) {
						newPlayerCoord.Y -= PLAYER_MOVEMENT_SPEED;
						tankMap->player->dir = UP;
					}

					if (!(currentWorldCoord.Y-WORLD_MOVEMENT_SPEED < 0) && (currentPlayerCoord.Y == (currentWorldCoord.Y + (VIEWPORT_HEIGHT / 2))))
						newWorldCoord.Y -= WORLD_MOVEMENT_SPEED;
					break;

				case ARROW_DOWN_KEY:
					/* Code for down arrow handling */
					if (!(currentPlayerCoord.Y + PLAYER_MOVEMENT_SPEED > WORLD_HEIGHT)) {
						newPlayerCoord.Y += PLAYER_MOVEMENT_SPEED;
						tankMap->player->dir = DOWN;
					}

					if (!(currentWorldCoord.Y+WORLD_MOVEMENT_SPEED > WORLD_HEIGHT-VIEWPORT_HEIGHT) && (currentPlayerCoord.Y == (currentWorldCoord.Y + (VIEWPORT_HEIGHT / 2))))
						newWorldCoord.Y += WORLD_MOVEMENT_SPEED;
					break;
				case ARROW_LEFT_KEY:
					/* Code for left arrow handling */
					if (!(currentPlayerCoord.X - PLAYER_MOVEMENT_SPEED < 0)) {
						newPlayerCoord.X -= PLAYER_MOVEMENT_SPEED;
						tankMap->player->dir = LEFT;
					}

					if (!(currentWorldCoord.X-WORLD_MOVEMENT_SPEED < 0) && (currentPlayerCoord.X == (currentWorldCoord.X + (VIEWPORT_WIDTH / 2))))
						newWorldCoord.X -= WORLD_MOVEMENT_SPEED;
					break;

				case ARROW_RIGHT_KEY:
					/* Code for right arrow handling */
					if (!(currentPlayerCoord.X + PLAYER_MOVEMENT_SPEED > WORLD_WIDTH)) {
						newPlayerCoord.X += PLAYER_MOVEMENT_SPEED;
						tankMap->player->dir = RIGHT;
					}

					if (!(currentWorldCoord.X+WORLD_MOVEMENT_SPEED > WORLD_WIDTH-VIEWPORT_WIDTH) && (currentPlayerCoord.X == (currentWorldCoord.X + (VIEWPORT_WIDTH / 2))))
						newWorldCoord.X += WORLD_MOVEMENT_SPEED;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			tankMap->TryMoveUnit(tankMap->player, newPlayerCoord);
		}

		std::list<Unit*>::iterator bulletIt;
		for (bulletIt = tankMap->player->bullets.begin(); bulletIt != tankMap->player->bullets.end();) {
			Unit *currentBullet = dynamic_cast<Unit*>(*bulletIt);
			if (currentBullet->lifeTime <= BULLET_MAX_LIFETIME) {
				COORD currentBulletCoord = currentBullet->position;
				COORD newBulletCoord = currentBulletCoord;
				switch (currentBullet->dir) {
				case UP:
					newBulletCoord.Y -= BULLET_MOVEMENT_SPEED;
					if (!(newBulletCoord.Y < 0))
						tankMap->TryMoveUnit(currentBullet, newBulletCoord);
					break;
				case DOWN:
					newBulletCoord.Y += BULLET_MOVEMENT_SPEED;
					if (!(newBulletCoord.Y > WORLD_HEIGHT))
						tankMap->TryMoveUnit(currentBullet, newBulletCoord);
					break;
				case LEFT:
					newBulletCoord.X -= BULLET_MOVEMENT_SPEED;
					if (!(newBulletCoord.X < 0))
						tankMap->TryMoveUnit(currentBullet, newBulletCoord);
					break;
				case RIGHT:
					newBulletCoord.X += BULLET_MOVEMENT_SPEED;
					if (!(newBulletCoord.X > WORLD_WIDTH))
						tankMap->TryMoveUnit(currentBullet, newBulletCoord);
					break;
				default:
					break;
				}
				currentBullet->lifeTime++;
				++bulletIt;
			} else {
				tankMap->EraseUnit(currentBullet);
				bulletIt = tankMap->player->bullets.erase(bulletIt);
			}
		}

		std::list<Unit*>::iterator enemyIt;
		for (enemyIt = tankMap->enemies.begin(); enemyIt != tankMap->enemies.end(); ++enemyIt) {
			Unit *currentEnemy = dynamic_cast<Unit*>(*enemyIt);
			COORD currentEnemyCoord = currentEnemy->position;
			COORD newEnemyCoord = currentEnemyCoord;
			//between 1 and 4
			int direction = rand() % 4 + 1;

			switch (direction) {
			case 1://UP
				newEnemyCoord.Y -= ENEMY_MOVEMENT_SPEED;
				if (!(newEnemyCoord.Y < 0))
					tankMap->TryMoveUnit(currentEnemy, newEnemyCoord);
				break;
			case 2://DOWN
				newEnemyCoord.Y += ENEMY_MOVEMENT_SPEED;
				if (!(newEnemyCoord.Y > WORLD_HEIGHT))
					tankMap->TryMoveUnit(currentEnemy, newEnemyCoord);
				break;
			case 3://LEFT
				newEnemyCoord.X -= ENEMY_MOVEMENT_SPEED;
				if (!(newEnemyCoord.X < 0))
					tankMap->TryMoveUnit(currentEnemy, newEnemyCoord);
				break;
			case 4://RIGHT
				newEnemyCoord.X += ENEMY_MOVEMENT_SPEED;
				if (!(newEnemyCoord.X > WORLD_WIDTH))
					tankMap->TryMoveUnit(currentEnemy, newEnemyCoord);
				break;
			default:
				break;
			}
		}

		tankMap->RenderMap(newWorldCoord.X, newWorldCoord.Y);
		endTime = clock();
		float diff = (float)endTime - (float)startTime;
		float seconds = diff / CLOCKS_PER_SEC;
		Sleep(FRAMETIME - (seconds*1000));

	} while (!tankMap->gameOver);
	if (tankMap->winner) {
		MessageBox(NULL, L"YOU WIN!!!!!", L"TankGame", MB_OK);
	} else {
		MessageBox(NULL, L"YOU LOSE!!!!!", L"TankGame", MB_OK);
	}
}

int main()
{
	MessageBox(NULL, L"Welcome!\nArrow keys: Move\nSpace: Shoot", L"TankGame", MB_OK);
	tankMap = new World();
	AdaptConsole(VIEWPORT_HEIGHT, VIEWPORT_WIDTH);
	//initialize array from file
	tankMap->InitializeFromFile();
	//render this array into the screen
	tankMap->RenderMap(0, 0);//Render from the top left
	StartGameLoop();
	return 0;
}
