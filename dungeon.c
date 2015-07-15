#include <assert.h>//Because I want to use it.
#include <stdlib.h>//For if I feel like mallocing something
#include <stdint.h>
#include <stdio.h>
#include <time.h>//For nanosleep
#include <string.h>//Actually just for the array tools. Strings are arrays, after all
#define DUNGEON_X 10
#define DUNGEON_Y 10
#define PLAYER_CHAR 'P'
#define WALL_CHAR 'X'
#define FLOOR_CHAR '*'//Using defines here for flexibility
#define TRAP_CHAR 'T'
#define TREASURE_CHAR '~'
#define MONSTER_CHAR 'a'
uint8_t sleepy; //For the benefit of machines. Why is it in the global scope when everything else is passed around through pointers? Reasons. That's why.
//Woo! Function prototypes!
void setUpDungeon(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t playerPos[2], uint8_t trapPos[2][2], uint8_t monsterPos[2], uint8_t treasurePos[2]);
void validateDungeon(char (*dungeon)[DUNGEON_X][DUNGEON_Y]);
void drawDungeon(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t playerPos[2], uint8_t trapPos[2][2],
	uint8_t monsterPos[2], uint8_t awake, uint8_t treasurePos[2]);
uint8_t isNear(uint8_t pos1[2], uint8_t pos2[2]);
int8_t parseUserInput(char (*dungeon)[DUNGEON_X][DUNGEON_Y], char inputChar, uint8_t (*playerpos)[2], uint8_t *awake, uint8_t (*trapLocs)[2][2]);
void printPrompt(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t awake, uint8_t playerPos[2], uint8_t trapPos[2][2],
	uint8_t monsterPos[2], uint8_t treasurePos[2]);
uint8_t move(char direction, char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t (*playerPos)[2], uint8_t *awake, uint8_t (*trapLocs)[2][2]);
void setOffTrap(uint8_t trapPos[2], uint8_t *awake, uint8_t (*trapLocs)[2][2]);
void doMonsterMove(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t playerPos[2], uint8_t (*monsterPos)[2], uint8_t *playing);

//Just sets up the basics of dungeon-ness. Since we're just using chars, we don't need to do any fancy struct stuff. This _is_ SuperBasic
void setUpDungeon(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t playerPos[2], uint8_t trapPos[2][2], uint8_t monsterPos[2], uint8_t treasurePos[2]){
	for(uint8_t iy = 0; iy < DUNGEON_Y; iy++){//Has to start with y because it's not possible to go back up after a \n
		for(uint8_t ix = 0; ix < DUNGEON_X; ix++){
			uint8_t currPos[2] = {ix, iy};
			(*dungeon)[ix][iy] = (ix == 0 || ix == DUNGEON_X - 1 || iy == 0 || iy == DUNGEON_Y - 1)?/*If it's an edge make it a wall*/
				WALL_CHAR :/*Otherwise, it's the floor, for now*/ FLOOR_CHAR;
				//Then we decide if the tile(?) needs to be something else
				if(!memcmp(currPos, playerPos, 2)){//Is the player here?
					(*dungeon)[ix][iy] = PLAYER_CHAR;//If they are, show it
				}
				else if(!memcmp(currPos, trapPos[0], 2) || !memcmp(currPos, trapPos[1], 2)){//|| and all that because hay dos traps. I'm actually
													//not planning to print the trap char, but just in case...
					(*dungeon)[ix][iy] = TRAP_CHAR;
				}
				else if(!memcmp(currPos, treasurePos, 2)){//Treasure really ought to be invisible, too
					(*dungeon)[ix][iy] = TREASURE_CHAR;//Something treasurey
				}
				else if(!memcmp(currPos, monsterPos, 2)){//The monster also needs to be placed. Invisibility code already exists, but is half-disabled
					(*dungeon)[ix][iy] = MONSTER_CHAR;
				}
		}

	}
}

void validateDungeon(char (*dungeon)[DUNGEON_X][DUNGEON_Y]){
	for(uint8_t ix = 0; ix < DUNGEON_X; ix++){//Just prints all the dungeon characters with no regard to what can and cannot be seen. Just for debugging
		for(uint8_t iy = 0; iy < DUNGEON_Y; iy++){
			printf(/*"(%u, %u) = */"  %c  "/*\n"*/, /*ix, iy,*/ (*dungeon)[ix][iy]);
		}
			puts("\n");
	}
	puts("\n");

}


void drawDungeon(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t playerPos[2], uint8_t trapPos[2][2],
	uint8_t monsterPos[2], uint8_t awake, uint8_t treasurePos[2]){
	for(uint8_t iy = 0; iy < DUNGEON_Y; iy++){//Has to start with y because it's not possible to go back up after a \n
		for(uint8_t ix = 0; ix < DUNGEON_X; ix++){
			uint8_t currPos[2] = {ix, iy};//Because memcmp likes arrays but dislikes their literals
			uint8_t isPlayerNearMonster = isNear(playerPos, monsterPos);
			uint8_t isCurrPosMonsterPos = !memcmp(monsterPos, currPos, 2);//! because of memcmp's return style. Checks the first 2 bytes of array element
			/* Lines can be uncommented for debugging
			printf("isPlayerNearMonster returned: %u\n", isPlayerNearMonster);
			printf("isCurrPosMonsterPos: %u\n", isCurrPosMonsterPos);
			printf("awake: %u\n", awake);
			*/
			if(isCurrPosMonsterPos && (!awake) && !isPlayerNearMonster){//Bug here fixed - caused by wrong awake at compile time
				/*If the current observed position is the monster's position AND the monster hasn't been
			 	*seen AND the player isn't adjacent to the monster
				*/
				printf("  %c  ", FLOOR_CHAR);//then put a floor character there,
			}
			else{//Otherwise, just print the character at the position
				printf("  %c  ", (*dungeon)[ix][iy]);
			}
		}
		puts("\n");
	}

}


/*
Detects if pos2 is on any of the cells adjacent to pos1, including pos1.
Diagrams: ***
	  *a*  * = scanned space, a = pos1, also scanned.
	  ***
Returns 1 if pos2 is in that region, returns 1 otherwise
*/
uint8_t isNear(uint8_t pos1[2], uint8_t pos2[2]){
	//Commented lines can be uncommented to aid debugging
	//puts("Running isNear\n");
	for(int8_t ix = -1; ix <= pos1[0]; ix++){//Goes through x values from pos1[0] - 1 to pos1[0] + 1
		for(int8_t iy = -1; iy <= pos1[1]; iy++){//See above, but with y and pos1[1]
			uint8_t currPos[2] = {pos1[0] + ix, pos1[1] + iy};//Because memcmp does not accept array literals
			//printf("(%d, %d) ", currPos[0], currPos[1]);
			if(!memcmp(currPos, pos2, 2)){//The ! is necessary because memcmp returns 0 if the two values match
				//printf("Coordinates: (%u, %u), (%u, %u)\n", pos1[0], pos1[1], pos2[0], pos2[1]);
				return 1;
			}

		}

	}
	//printf("Coordinates: (%u, %u), (%u, %u)\n", pos1[0], pos1[1], pos2[0], pos2[1]);
	return 0;
}
//Returns 1 if everything went fine; 0 if invalid movement; -1 if invalid command;
int8_t parseUserInput(char (*dungeon)[DUNGEON_X][DUNGEON_Y], char inputChar, uint8_t (*playerPos)[2], uint8_t *awake, uint8_t (*trapLocs)[2][2]){
	switch (inputChar){
		case 'w':
			return move('w', dungeon, playerPos, awake, trapLocs);
		case 'a':
			return move('a', dungeon, playerPos, awake, trapLocs);
		case 's':
			return move('s', dungeon, playerPos, awake, trapLocs);
		case 'd':
			return move('d', dungeon, playerPos, awake, trapLocs);
		default:
			puts("Invalid command.");
			return -1;
	}

}

//Returns 1 if movement successful; 0 otherwise
uint8_t move(char direction, char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t (*playerPos)[2], uint8_t *awake, uint8_t (*trapLocs)[2][2]){
	uint8_t dirnum;
	uint8_t targetPos[2];
	switch (direction){
		case 'w':
			dirnum = 1;
			break;
		case 'a':
			dirnum = 2;
			break;
		case 's':
			dirnum = 3;
			break;
		case 'd':
			dirnum = 4;
			break;
		default:
			dirnum = 0;
			break;

	}
	if(dirnum){
		targetPos[0] = (*playerPos)[0] + (dirnum == 1? 0 : dirnum == 2? -1 : dirnum == 3? 0 : dirnum == 4? 1 : 0);
		targetPos[1] = (*playerPos)[1] + (dirnum == 1? -1 : dirnum == 2? 0 : dirnum == 3? 1 : dirnum == 4? 0 : 0);
		uint8_t targetChar = (*dungeon)[targetPos[0]][targetPos[1]];
		//printf("Target position: (%u, %u) : %c\n", targetPos[0], targetPos[1], (*dungeon)[targetPos[0]][targetPos[1]]);//Don't mind me. Here for debugging
		if(targetChar == FLOOR_CHAR ||
			targetChar == TREASURE_CHAR || targetChar == TRAP_CHAR){


			(*playerPos)[0] = targetPos[0], (*playerPos)[1] = targetPos[1];
			if (targetChar == TRAP_CHAR) setOffTrap(targetPos, awake, trapLocs);
			return 1;
		}
		else{
			if(targetChar == MONSTER_CHAR) puts("Why would you want to go there? That's the monster!");
			else if(targetChar == WALL_CHAR) puts("That's a wall. You can't walk through it.");

			return -1;
		}

	}

	return 0;
}

void printPrompt(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t awake, uint8_t playerPos[2],
	uint8_t trapPos[2][2], uint8_t monsterPos[2], uint8_t treasurePos[2]){//trapPos useful if some kind of detector is added

	uint8_t canSeeMonster = isNear(playerPos, monsterPos) || awake;
	uint8_t canSeeTreasure = isNear(playerPos, treasurePos);
	uint8_t canSeeTrap = 0;//Going to implement it anyway just in case I ever want trap detectors
	printf("Map key: ");
	if (canSeeMonster) printf("%c : The monster | ", MONSTER_CHAR);
	if (canSeeTrap) printf("%c : A trap | ", TRAP_CHAR);
	if (canSeeTreasure) printf("%c : The treasure | ", TREASURE_CHAR);
	printf("%c : A wall | %c : The floor | %c : You\n", WALL_CHAR, FLOOR_CHAR, PLAYER_CHAR);
	puts("Controls: w : Go up | a : Go left | s : Go down | d : Go right");


}


void setOffTrap(uint8_t trapPos[2], uint8_t *awake, uint8_t (*trapLocs)[2][2]){
	*awake = 1;//The monster wakes up
	uint8_t nullloc[2] = {0, 0};
	for(uint8_t i = 0; i < 2; i++){
		if(!memcmp(trapLocs[i], trapPos, 2)){
			for(uint8_t i2 = 0; i2 < 2; i2++){
				(*trapLocs)[i2][0] = nullloc[0];
				(*trapLocs)[i2][1] = nullloc[1];
			}
		}
	}
	puts("You stepped on a trap. The monster has awoken. Sheeeeeeet.");
	if (sleepy){
		struct timespec delayTime;
		delayTime.tv_sec = 2;
		delayTime.tv_nsec = 500*1000*1000;
		nanosleep(&delayTime, NULL);
	}
}

void doMonsterMove(char (*dungeon)[DUNGEON_X][DUNGEON_Y], uint8_t playerPos[2], uint8_t (*monsterPos)[2], uint8_t *playing){
	a.flija/foij//Write this function



}


int main(int argc, char *argv[]){
	sleepy = argc != 1? 0 : 1;//So that if a machine is playing, sleeps can be removed
	struct timespec delayTime;//
	delayTime.tv_sec = 0;//These three lines create something that nanosleep can use to sleep. Details can be changed to change sleep time
	delayTime.tv_nsec = 500*1000*1000;//Starting at 500,000,000 nanoseconds (0.5s). Can be reassigned.
	puts("This is an in-development version. The treasure and traps are always visible.");
	char dungeon[DUNGEON_X][DUNGEON_Y];
	uint8_t playerPos[2] = {1, 1};//Starting position for the player
	uint8_t playing = 1; //So we don't instantly exit
	uint8_t monsterPos[2] = {2, 2}; //Starting monster position - randomisation incoming
	uint8_t awake = 0; //The monster starts invisible. This doubles as 'is the monster active?'
	uint8_t trapPos[2][2] = {{1, 2}, {3, 5}}; //Two traps to wake the monster - will be randomised
	uint8_t treasurePos[2] = {2, 1}; //The treasure was once in the same place as the monster. Randomisation will need to ensure that this is never the case
	setUpDungeon(&dungeon, playerPos, trapPos, monsterPos, treasurePos);//Because setUpDungeon takes a pointer, and a bunch of positions to tell it where stuff goes
	//validateDungeon(&dungeon);//As does validateDungeon. Commented because reasons
	if (sleepy) nanosleep(&delayTime, NULL);//Wait a moment
	while(playing){//If you're not playing anymore, the game really ought to end
		system("clear");//We are now officially *NIX specific
		uint8_t isValidCommand = 0;//So that if the input is invalid, it can be ignored and retried without redrawing
		uint8_t ignoreNext = 0;//For invalid strings, helps prevent ignorance of next valid input
		int8_t parseReturn;
		setUpDungeon(&dungeon, playerPos, trapPos, monsterPos, treasurePos);//Because setUpDungeon takes a pointer to a char[][]
										    //and a bunch of positions so that all things are in their rightful places
		drawDungeon(&dungeon, playerPos, trapPos, monsterPos, awake, treasurePos);//drawDungeon must know all things' positions
		printPrompt(&dungeon, awake, playerPos, trapPos, monsterPos, treasurePos);
		char text[3];//Command char, \n and \0
		while(!isValidCommand){
			fgets(text, 3, stdin);//Technically, this reads a file. stdin is a file, though. Uses a pointer, the number of characters you want and file
			if((text[1] == '\n' && text[2] == '\0') || text[1] == '\0'){//If the only input characters are the command, \n and EOF, or the command is \nEOF
									//in the event that the previous command was invalid and of the correct length type.
									//Rejects strings terminated with ^D but no \n, and glitches out, but that's not the kind of thing
				if(ignoreNext || text[0] == '\n'){	//any normal person would be inputting, anyway. Machines will learn what constitutes valid, too
					ignoreNext = 0;
				}
				else{
					parseReturn = parseUserInput(&dungeon, text[0], &playerPos, &awake, &trapPos);
					isValidCommand = parseReturn == 1? 1 : 0;
				}
			}
			else{
				if(!ignoreNext){
				puts("Invalid command.");
				ignoreNext = 1;
				if (sleepy) nanosleep(&delayTime, NULL);
				}
			}
		}
		if (sleepy) nanosleep(&delayTime, NULL);
		doMonsterMove(&dungeon, playerPos, &monsterPos, &playing);
	}
	return 0;
}
