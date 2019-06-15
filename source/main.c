/*---------------------------------------------------------------------------------



---------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>

#define WIDTH 32
#define HEIGHT 24

int x = 0;
int y = 0;
char charToDraw = ' ';
char screen[24][32] = {"+------------------------------+",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"|                              |",
"+------------------------------+"};
int main(void) {
  // Set up text on bottom screen
	consoleDemoInit();

  // Game loop
  while(1) {
    for (y = 0; y < HEIGHT; y++) {
      for(x = 0; x < WIDTH; x++) {
        iprintf("\x1b[%i;%iH%c", y, x, screen[y][x]);
      }
    }
		swiWaitForVBlank();
	}

}
