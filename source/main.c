/*---------------------------------------------------------------------------------

A DS port of the text based puzzle platformer SuperSerifBros
Original version at http://foon.uk/farcade/ssb/

Bobberto1995 2019

---------------------------------------------------------------------------------*/
#include <nds.h>
#include <fat.h>

#include <stdio.h>
#include <string.h>

#define WIDTH 32
#define HEIGHT 24

#define TERM_WIDTH 80
#define TERM_HEIGHT 25

void updatePf(char* arr);
char getPfCh(int x, int y);
void setupDsScreen();
void replace(char a, char b);
void swap(char a, char b);
char* getStString();
int getBestLevel();
void setBestLevel(int level);
void clearBestLevel();
void load();
void copyA(char** to, char** from);
void frameLoop();
void doFrame1();
void doFrame8();
bool isEnemy(char x);
bool canfall(char x);
bool conveys(char x);
bool probe(int x, int y, char ch);
void die();
void win();
void pollInput();
void checkKeyPressed();
void checkKeyReleased();
int getDsScreenCoordinates();

// Stuff for C
char ar[TERM_HEIGHT][TERM_WIDTH];
char arp[TERM_HEIGHT][TERM_WIDTH];
char temp[TERM_HEIGHT][TERM_WIDTH];

// Stuff for DS
char dsScreenString[HEIGHT * WIDTH + 1];
PrintConsole topScreen;
PrintConsole bottomScreen;

// Globals
int stop_loop = 0;
int moneyl = 0;
int level = 1;
int frames = 0;
bool got_all_money;
int bestlevel = 1;
char* st;
bool tired = false;
int tired_i = 0;
bool rflag = false;
bool moved = false;
int dir;
bool gr;
char ob;
char od;
char fl;
int key_left, key_right, key_up, key_down;
bool key_r, key_start, key_select;
int kd_left, kd_right, kd_up, kd_down;
bool can_advance;
char sp = ' ';
int dx, dy;

int main(void) {
  // Set up consoles for printing
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);
	
	consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
  
  // Set up filesystem
  fatInitDefault();
  
  // Load level
  load();
  // Game loop
  while(1) {
    if (!stop_loop) {
      frameLoop();
    }
    
    // run at 10FPS
    int i;
    for (i = 0; i < 6; i++) {
      checkKeyReleased();
      checkKeyPressed();
      swiWaitForVBlank();
    }
	}

}
/**
* updatePf
* Updates the playfield from the cache and draws to the screen
* (maybe should also pass in cache?)
* @arg arr the array of arrays of chars representing the playfield
*/
void updatePf(char* arr) {
  int y;
  int x;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      if (arp[y][x] != ar[y][x]) {
        char ch = getPfCh(x, y);
        if (ch == 'I' && !moved) {
          // set color to blue
        }
        // update to screen
      }
    }
  }
}

/**
* getPfCh
* @arg x the x coordinate (column)
* @arg y the y coordinate (row)
* @return the character at the given position
*/
char getPfCh(int x, int y) {
  return ar[y][x];
}

/**
* setupDsScreen
* Move a DS screen sized slice of the playfield into the DS screen string for
* printing.
*/
void setupDsScreen(int xOff, int yOff) {
  int x;
  int y;
  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      dsScreenString[y * WIDTH + x] = getPfCh(x + xOff, y + yOff);
    }
  }
  dsScreenString[WIDTH * HEIGHT] = '\0';
}

/**
* replace
* Replaces all instances of a character in the playfield with another
* @arg a the character to be replaced
* @arg b the new character to replace a
*/
void replace(char a, char b) {
  int x;
  int y;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      if (ar[y][x] == a) {
        arp[y][x] = b;
      }
    }
  }
}

/**
* swap
* Swaps two characters in the playfield. All instances of a become b and all
* instances of b become a.
* @arg a the first character
*/
void swap(char a, char b) {
  int x;
  int y;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      if (ar[y][x] == a) {
        arp[y][x] = b;
        ar[y][x] = ' ';
      } else if (ar[y][x] == b) {
        arp[y][x] = a;
        ar[y][x] = ' ';
      }
    }
  }
}

/**
* getStString
* Gets the status string. This information will be displayed on the DS's top
* screen in this port.
* @return the status string showing level and money information
*/
char* getStString() {
  static char str[3 * WIDTH];
  str[0] = '\0';
  if (stop_loop == 1) {
    strcat(str, "\nYOU'RE DEAD; PRESS [R] TO RE-TRY");
  } else if (stop_loop == 2) {
    strcat(str, "NICELY DONE! [START] FOR NEXT\nLEVEL OR [R] TO RE-TRY\n");
  } else {
    strcat(str, "\n\n");
  }
  strcat(str, "[Level %i][$'s left: %i]");
  return str;
}

/**
* fetchLevel
* Loads a level from the filesystem.
* @arg the index of the level to return
* @return a string containing the level data
*/
char* fetchLevel(int level) {
  char fileName[50];
  fileName[0] = '\0';
  sprintf(fileName, "/DATA/bobberto1995/superserifbrosds/levels/%i.txt", level);
  FILE* levelFile = fopen(fileName, "r");
  if (!levelFile) {
    consoleSelect(&bottomScreen);
    iprintf("Error opening file!");
    iprintf("Filename: ");
    iprintf(fileName);
    while(1) {
      swiWaitForVBlank();
    }
  }
  static char levelData[TERM_WIDTH * TERM_HEIGHT + 1];
  int c;
  int index = 0;
  while ((c = fgetc(levelFile)) != EOF && index < TERM_WIDTH * TERM_HEIGHT) {
    levelData[index] = (char) c;
    index++;
  }
  fclose(levelFile);
  return levelData;
}

/**
* getBestLevel
* loads from the save file.
* @return the index of the highest level you've reached
*/
int getBestLevel() {
  return 1;
}

/**
* setBestLevel
* writes to the save file.
* @arg the index of the highest level you've reached
*/
void setBestLevel(int level) {
  return;
}

/**
* clearBestLevel
* Erases the save file. Sets your best level back to 1
*/
void clearBestLevel() {
  setBestLevel(1);
}

/**
* load
* Loads a level into the playfield and resets status parameters
*/
void load() {
  frames = 0;
  if(level > bestlevel) {
    bestlevel = level;
    setBestLevel(level);
    // updateLinks();
  }
  got_all_money = false;
  moved = false;
  // clearTimeout(frt);
  stop_loop = 0;
  char* str = fetchLevel(level);
  int x = 0;
  int y = 0;
  int l = 0;
  while (l < 81 * 25) {
  char ch = str[l];
    if(ch == '\n') {
      y++;
      x = 0;
    } else {
      ar[y][x] = ch;
      x++;
    }
    l++;
  }
}

/**
* copyArpToAr
* copies arp to ar
*/
void copyArpToAr() {
  int x = 0;
  int y = 0;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      ar[y][x] = arp[y][x];
    }
  }
}

/**
* copyArToArp
* copies ar to arp
*/
void copyArToArp() {
  int x = 0;
  int y = 0;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      arp[y][x] = ar[y][x];
    }
  }
}

/**
* copyArToTemp
* copies ar to temp
*/
void copyArToTemp() {
  int x = 0;
  int y = 0;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      temp[y][x] = ar[y][x];
    }
  }
}

/**
* copyTempToArp
* copies temp to arp
*/
void copyTempToArp() {
  int x = 0;
  int y = 0;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      arp[y][x] = temp[y][x];
    }
  }
}

/**
* frameLoop
* In this case, just one frame. main() handles timing.
*/
void frameLoop() {
  pollInput();
  copyArToArp();
  
  if (rflag) {
    swap('{', '}');
    swap('[', ']');
    swap('<', '>');
    swap('(', ')');
    rflag = false;
  }
  
  doFrame1();
  if (frames == 8) {
    doFrame8();
    frames = 0;
  }
  frames++;
  
  copyArToTemp();
  copyArpToAr();
  copyTempToArp();
  
  
  // Draw game to top screen
  int coords = getDsScreenCoordinates();
  int xCoord = coords & 255;
  int yCoord = (coords >> 8) & 255;
  setupDsScreen(xCoord, yCoord);
  consoleSelect(&topScreen);
  iprintf("\x1b[0;0H");
  iprintf(dsScreenString);
  
  // Draw status to bottom screen
  st = getStString();
  consoleSelect(&bottomScreen);
  iprintf("\x1b[0;0H");
  iprintf(getStString(), level, moneyl);
  iprintf("\n");
  iprintf("use dpad to move\ncollect all $'s to open the exit e -> E\nthe rest is up to you");
  if (moneyl == 0) {
    got_all_money = true;
  }
  if (tired) {
    tired_i++;
    if (tired_i >= 2) {
      tired_i = 0;
      tired = false;
    }
  }
}

/**
* isEnemy
* @arg x the char to test
* @return true if x is an enemy
*/
bool isEnemy(char x) {
  switch (x) {
    case '[':
    case ']':
    case '{':
    case '}':
    case '%':
      return true;
  }
  return false;
}

/**
* canfall
* @arg x the char to test
* @return true if x should be affected by gravity
*/
bool canfall(char x) {
  switch(x) {
    case 'I':
    case '[':
    case ']':
    case 'O':
    case '%':
    case '$':
      return true;
  }
  return false;
}

/**
* conveys
* @arg x the char to test
* @return true if x should be carried by conveyor belts
*/
bool conveys(char x) {
  switch(x) {
    case 'I':
    case '[':
    case ']':
    case 'O':
    case '%':
    case '$':
      return true;
  }
  return false;
}

bool probe(int x, int y, char ch) {
  if(y >= 25 || y < 0 || x >= 80 || x < 0) return true;
		
  char ob = ar[y][x];
  char ob1 = arp[y][x];
  
  switch(ch) {
    case 'I':
      if(ob == 'E') {
        win();
        return false;
      } else if(ob == '$') {
        arp[y][x] = ' ';
        moneyl--;
        return false;
      } else if(ob == '0') {
        return true;
      } else if(isEnemy(ob)) {
        die();
        return true;
      } else if(ob == 'O') {
        int dir = 0;
        if(key_left) dir--;
        else if(key_right) dir++;
        if(dir) {
          if(probe(x,y+1,ob) && !probe(x+dir,y,ob) && ar[y][x-dir]=='I') {
            arp[y][x+dir] = ob;
            arp[y][x] = ' ';
            
            if(ar[y+1][x]==';')
              arp[y+2][x]=' ';
              
            tired = true;
            return false;
          }
          else return true;
        }
        else return true;
      }
      break;
  
    case '<':
    case '>':
      if(ob == 'I') return false;
      break;
    case '}': 
    case '{': 
    case ']':
    case '[':
      dir = (ch=='}'||ch==']')?1:-1;
      if(ob == 'O' || ob == '$') {
        if( probe(x,y+1,ob) && !probe(x+dir,y,ob)) {
          arp[y][x+dir] = ob;
          return false;
        }
      } else if(ob == 'I') {
        die();
        return false;
      }
      break;
      
    default:
      break;
  }
  return !(ob==' ' && ob1== ' ');
}

// -- 1 -- 1 -- 1 --
void doFrame1() {
  int x, y;
  bool dead = true;
  moneyl = 0;
  if(key_left) {
    key_left = 2;
  }
  if(key_right) {
    key_right = 2;
  }
  if(key_up) {
    key_up = 2;
  }
  if(key_down) {
    key_down = 2;
  }
  
  for(y=0; y<TERM_HEIGHT; y++) {
    for(x=0;x<80;x++) {
      char ch = ar[y][x];
      
      if(canfall(ch) && y == 24) {
        arp[y][x]=sp;
      } else if (ch - 48 >= 1 && ch - 48 <= 9) {
        if (ar[y-1][x] != sp) {
          arp[y][x] = ch - 49;
        }
      } else {
        switch(ch) {
          case '0':
            arp[y][x]=sp;
            break;
            
          case '%':
            if(y<24 && ar[y+1][x]==';' && arp[y][x]=='%') {
              arp[y][x]=sp;
              if(y<23) arp[y+2][x]=ch;
            }
            else if(!probe(x,y+1,ch)) {
              arp[y+1][x]=ch;
              arp[y][x]=sp;
            }
            else if((y<24) && (ar[y+1][x]=='I')) {
              arp[y+1][x]=ch;
              arp[y][x]=sp;
            }
            break;
            
          case ':':
            if(y > 0) {
              if(ar[y-1][x]=='O' || ar[y-1][x]=='%') arp[y][x]=';';
              else if(ar[y-1][x]=='X' || ar[y-1][x]=='.')
                arp[y][x]='.';
            }
            break;
            
          case ';':
            if(y > 0) {
              if(ar[y-1][x]!='O' && ar[y-1][x]!='%')
                arp[y][x]=':';
            }
            break;
            
          case 'O':
            if(y<24 && ar[y+1][x]==';' && arp[y][x]=='O') {
              arp[y][x]=sp;
              if(y<23) arp[y+2][x]=ch;
            }
            else if(!probe(x,y+1,ch)) {
              arp[y+1][x]=ch;
              ar[y][x]=sp;
              arp[y][x]=sp;
            }
            break;	
            
          case '.':
            arp[y][x]=':';
            break;
          
          case '&':
          case '?':
            for(dx=-1;dx<=1;dx++) {
              for(dy=-1;dy<=1;dy++) {
                if(ar[y+dy][x+dx]=='0') {
                  arp[y][x]='0';
                }
              }
            }
            ch = '%';
            break;
            
          case '$':
            moneyl++;
            if(!probe(x,y+1,ch)) {
              arp[y+1][x]=ch;
              arp[y][x]=sp;
            }
            else if((y<24) && (ar[y+1][x]=='I')) {
              arp[y][x]=sp;
            }
            break;
            
          case 'T':
            if(ar[y-1][x]=='I') {
              if(x>0 && key_left && arp[y-1][x-1]=='I' && !probe(x-1,y,ch)) {
                arp[y][x-1]=ch;
                arp[y][x]=sp;
              }
              else if(x<79 && key_right && arp[y-1][x+1]=='I' && !probe(x+1,y,ch)) {
                arp[y][x+1]=ch;
                arp[y][x]=sp;
              }
            }
            break;
            
          case '¦':
            if(y > 0 && ar[y-1][x]=='.') {
              arp[y][x]='A';
            }
            break;
            
          case 'A':
            if(y > 0 && (ar[y-1][x]==':' || ar[y-1][x]=='.') ) {
              if(!probe(x,y+1,'O')) {
                arp[y+1][x]='O';
                arp[y][x]='¦';
              }
            }
            break;
            
          case 'I':
            dead = false;
            if(!probe(x,y+1,ch)) {
              arp[y+1][x]=ch;
              arp[y][x]=sp;
            } else {
              fl = ar[y+1][x];
              if(fl=='('||fl==')'||tired) {
                // do nothing
              } else if(key_left) {
                if(!probe(x-1,y,ch)) {
                  arp[y][x-1] = ch;
                  arp[y][x] = sp;
                }
                else if(!probe(x-1,y-1,ch)) {
                  arp[y-1][x-1] = ch;
                  arp[y][x] = sp;
                }
              } else if(key_right) {
                if(!probe(x+1,y,ch)) {
                  arp[y][x+1] = ch;
                  arp[y][x] = sp;
                }
                else if(!probe(x+1,y-1,ch)) {
                  arp[y-1][x+1] = ch;
                  arp[y][x] = sp;
                }
              } else if(key_up) {
                if(y != 0) {
                  if(ar[y-1][x]=='-' && !probe(x,y-2,ch)) {
                    arp[y-2][x]='I';
                    arp[y][x]=sp;
                  }
                  else if(ar[y+1][x]=='"') {
                    if(!probe(x,y-1,ch)) {
                      arp[y+1][x]=sp;
                      arp[y][x]='"';
                      arp[y-1][x]=ch;
                    }
                  }
                }
              } else if(key_down) {
                if(y != 24) {
                  if(ar[y+1][x]=='-' && !probe(x,y+2,ch)) {
                    arp[y+2][x]='I';
                    arp[y][x]=sp;
                  } else if(ar[y+1][x]=='"') {
                    if(!probe(x,y+2,'"')) {
                      arp[y+2][x]='"';
                      arp[y+1][x]=ch;
                      arp[y][x]=sp;
                    }
                  } else if(ar[y+1][x]=='~') {
                    replace('@','0');
                  } else if(ar[y+1][x]=='`') {
                    rflag = true;
                  }
                }
              }
            }
            break;
            
          case 'x':
            if(ar[y-1][x]!=sp) {
              arp[y-1][x]=sp;
              arp[y][x]='X';
            }
            break;
            
          case 'X':
            arp[y][x]='x';
            break;
            
          case 'e':
            if(got_all_money)
              arp[y][x]='E';
            break;
          case 'E':
            if(!got_all_money)
              arp[y][x]='e';
            break;
            
          case ')':
          case '(':
            ob = ar[y-1][x];
            if(conveys(ob)) {
              dir = (ch==')')?1:-1;
              if(!probe(x+dir,y-1,ob)) {
                arp[y-1][x]=sp;
                arp[y-1][x+dir]=ob;
              }
            }
            break;
            
          case '<':
          case '>':
            dir = (ch=='<')?-1:1;
            if(!probe(x+dir,y,ch)) {
              arp[y][x]=sp;
              arp[y][x+dir]=ch;
              if(y > 0) {
                ob = ar[y-1][x];
                if(conveys(ob) && !probe(x+dir,y-1,ob) ) {
                  arp[y-1][x] = sp;
                  arp[y-1][x+dir] = ob;
                }
              }
            }
            break;
            
          case '{':
          case '}':
          case '[':
          case ']':
            dir = (ch=='['||ch=='{')?-1:1;
            gr = (ch=='['||ch==']');
            od = (dir==1)?(gr?'[':'{'):(gr?']':'}');
            
            fl = ar[y+1][x];
              
            if(!(gr && (fl=='('||fl==')'))) {
              if(probe(x+dir,y,ch) && ((!gr)||probe(x,y+1,ch)) )
                arp[y][x]=od;
              else if(gr && !probe(x,y+1,ch)) {
                arp[y][x]=sp;
                arp[y+1][x]=ch;
              }
              else {
                arp[y][x]=sp;
                arp[y][x+dir]=ch;
              }
            }
            break;					
        }
      }
    }
  }
  if(dead) {
    die();
  }
}

// -- 8 -- 8 -- 8 --
void doFrame8() {
  int x,y;
  for(y=0;y<25;y++) for(x=0;x<80;x++) {
    char ch = ar[y][x];
    switch(ch) {
      case '=':
        if(y > 0) {
          ob = ar[y-1][x];
          if(ob != sp && ob != 'I') {
            if(!probe(x,y+1,ob))
              arp[y+1][x] = ob;
          }
        }
        break;
        
      case 'd':
      case 'b':
        od = (ch=='d')?'b':'d';
        dir = (ch=='d')?-1:1;
        if(probe(x+dir,y,ch))
          arp[y][x]=od;
        else {
          arp[y][x]=sp;
          arp[y][x+dir]=ch;
        }
        break;					
    }
  }
}

void die() {
  stop_loop = 1;
}
void win() {
  stop_loop = 2;
  can_advance = true;
}

void pollInput() {
  if(kd_left == 2 && key_left == 2) {
    kd_left = 0;
    key_left = 0;
  }
  if(kd_up == 2 && key_up == 2) {
    kd_up = 0;
    key_up = 0;
  }
  if(kd_right == 2 && key_right == 2) {
    kd_right = 0;
    key_right = 0;
  }
  if(kd_down == 2 && key_down == 2) {
    kd_down = 0;
    key_down = 0;
  }
}

void checkKeyPressed() {
  scanKeys();
  if (keysHeld() & KEY_LEFT) {
    key_left = 1;
    kd_left = 1;
  }
  if (keysHeld() & KEY_RIGHT) {
    key_right = 1;
    kd_right = 1;
  }
  if (keysHeld() & KEY_UP) {
    key_up = 1;
    kd_up = 1;
  }
  if (keysHeld() & KEY_DOWN) {
    key_down = 1;
    kd_down = 1;
  }
  if ((keysHeld() & KEY_SELECT) && !key_select) {
    key_start = true;
    can_advance = false;
    level++;
    load();
  }
  if (((keysHeld() & KEY_START) && can_advance && !key_start)) {
    key_start = true;
    can_advance = false;
    level++;
    load();
  }
  if (keysHeld() & KEY_R) {
    load();
  }
}

void checkKeyReleased() {
  scanKeys();
  if ((keysHeld() & KEY_LEFT) == 0) {
    if (key_left == 2) {
      key_left = 0;
    } else {
      kd_left = 2;
    }
  }
  if ((keysHeld() & KEY_RIGHT) == 0) {
    if (key_right == 2) {
      key_right = 0;
    } else {
      kd_right = 2;
    }
  }
  if ((keysHeld() & KEY_UP) == 0) {
    if (key_up == 2) {
      key_up = 0;
    } else {
      kd_up = 2;
    }
  }
  if ((keysHeld() & KEY_DOWN) == 0) {
    if(key_down == 2) {
      key_down = 0;
    } else {
      kd_down = 2;
    }
  }
  if ((keysHeld() & KEY_START) == 0) {
    key_start = false;
  }
  if ((keysHeld() & KEY_SELECT) == 0) {
    key_select = false;
  }
}

int getDsScreenCoordinates() {
  int centerX = 0;
  int centerY = 0;
  int numCharacters = 0;
  int x = 0;
  int y = 0;
  for (y = 0; y < TERM_HEIGHT; y++) {
    for (x = 0; x < TERM_WIDTH; x++) {
      if(ar[y][x] == 'I') {
        centerX += x;
        centerY += y;
        numCharacters++;
      }
    }
  }
  centerX /= numCharacters;
  centerY /= numCharacters;
  centerX -= WIDTH / 2;
  centerY -= HEIGHT / 2;
  if (centerX < 0) {
    centerX = 0;
  }
  if (centerX >= TERM_WIDTH - WIDTH) {
    centerX = TERM_WIDTH - WIDTH - 1;
  }
  if (centerY < 0) {
    centerY = 0;
  }
  if (centerY >= TERM_HEIGHT - HEIGHT) {
    centerY = TERM_HEIGHT - HEIGHT - 1;
  }
  return centerX + (centerY << 8);
}