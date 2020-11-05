#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h> // for random()
#include <math.h>
#include "screen.h"
#include "tty.h"
#include "snake.h"
#include "midi.h"

//TODO: 1. mid files
//              2. through the side tunnel
//              3.strange unstable  displays
//              4. combine the files to lower than 9
//              5. document all the features
//              6. if have time, write a little "How To Play" on the slash page


// The characters to be shown and their colors.
char scr[40][16];
char color[40][16];

extern uint8_t shrt[], pause[], medley[];
extern uint8_t veryshort[];
extern uint8_t fruit[];
extern uint8_t moneymid[];
extern uint8_t death[];
MIDI_Player *mp;

// Data variables for the game...
int px;
int py;
int dx;
int dy;
int prevdx;
int prevdy;

int gcol = 0xe0;//color of ghost
int map = 0xc0;//color of map
int bean = 0xd0;
//ghost
int gx;
int gy;
int prevgx;
int prevgy;
//pacman
char pacman;

int score;
int best = 0;
enum { SPLASH, RUNNING} phase = SPLASH;
int splash_ticks;

// Print a message at screen coordinate x,y in color c.
void msg(int x, int y, int c, const char *s)
{
  int n;
  int len = strlen(s);
  for(n=0; n<len && n+x<40; n++) {
    color[x+n][y] = c;
    scr[x+n][y] = s[n];
  }
}

void getfruit(void)
{
  score +=50;
  init_TIM3(50);
  render();
  if (mp->nexttick >= MAXTICKS)
      mp = midi_init(fruit);
}

void collision(void)
{
    msg(px,py,0x90, "X");
    char buf[30];
    msg(7,8, 0x0f, "     G A M E   O V E R     ");
    msg(7,9, 0x0f, "      Your Score:          ");
    sprintf(buf, "%d", score);
    msg(26,9,0xaf,buf);
    msg(7,10, 0x0f, " Press any key to restart  ");
    msg(7,11,0x0f,"         (｡ì _ í｡)         ");
    init_TIM3(50);
    render();
    if (mp->nexttick >= MAXTICKS)
        mp = midi_init(death);
   if (score > best) {
       best = score;
       msg(7,8, 0x0f, "  You got the new record!  ");
       msg(7,9, 0x0f, "      Your Score:          ");
       sprintf(buf, "%d", score);
       msg(26,9,0xaf,buf);
       msg(7,10, 0x0f, " Press any key to restart  ");
       msg(7,11,0x0f,"         (｡ì _ í｡)         ");
   }
}

// Initialize the game data structures.
void init(void)
{

  msg(0,0,map,";==================;===================;");
  msg(0,1,map,"| ................ | ................. |");
  msg(0,2,map,"| . === . ;====; ..... ===== . ===== . |");
  msg(0,3,map,"| ....... |    | . | ................. |");
  msg(0,4,map,"!=====; . |    | . | . ===;=== . ;=====!");
  msg(0,5,map,"      | . !====! ........ | .... |");
  msg(0,6,map,"======! .......... | . ===!==; . !======");
  msg(0,7,map,"        . ;========! ........| .        ");
  msg(0,8,map,"======; . |           . ;=   | . ;======");
  msg(0,9,map,"      | . |         | . |    | . |      ");
  msg(0,10,map,";=====! . !=====!   | . !====! . !=====; ");
  msg(0,11,map,"| .................................... |");
  msg(0,12,map,"| . === . ============== . ========= . |");
  msg(0,13,map,"| .................................... | ");
  msg(0,14,map,"!======================================!");
  for(int y=0; y<15; y++)
    for(int x=0; x<40; x++) {
      if (scr[x][y] == '.')
          color[x][y] = bean;
    }

  px = 26;
  py = 9;
  scr[px][py] = 'o';
  color[px][py] = 0x60;

  //ghost *v*
  gx = 13;
  gy =9;
  msg(gx-1, gy,gcol, "*v*");

    //pacman
    px = 13;
    py = 13;
    scr[px][py] = '<';
    color[px][py] = 0xb0;//yellow

    //status
    score = 0;
    char buf[30];
    sprintf(buf, "SCORE:%d  ", score);
    msg(3,15, 0xf0, buf);
    sprintf(buf, "BEST:%d  ", best);
    msg(27,15, 0xf0, buf);

  dx=1;
  dy=0;
  pacman = '<';

}

void render(void)
{
  int x,y;
  home();
  int col = color[0][0];
  fgbg(col);
  for(y=0; y<16; y++) {
    setpos(0,y);
    for(x=0; x<40; x++) {
      if (color[x][y] != col) {
        col = color[x][y];
        fgbg(col);
      }
      putchar(scr[x][y]);
    }
  }
  fflush(stdout);
}

// Display the initial splash screen.
void splash(void)
{
  clear();
  int x,y;
  for(y=0; y<16; y++)
    for(x=0; x<40; x++) {
      scr[x][y] = ' ';
      color[x][y] = 0x70;
    }
  msg(7,6, 0x0c, "        ('w')        ");
  msg(7,7, 0x0c, "  Extra Cute PacMan  ");
  msg(7,8,0x0c, "    Press Any Key    ");
  msg(7,9,0x0c, "                     ");
  render();
}

void move(void)
{
   if (scr[px-dx][py-dy] == '.') {
       score++;
   }
//   if (px ==0){
//       scr[39][7] = ' ';}
//   if (px == 39){
//       scr[0][7] = ' ';}
//   else
       scr[px-dx][py-dy] = ' '; // erase end of tail
    scr[px][py] = pacman; // draw new head
    if ((px+py)%3==1)
        scr[px][py] =  '-';
    color[px][py] = 0xb0;

    char buf[30];
    sprintf(buf, "SCORE:%d  ", score);
    msg(3,15, 0xf0, buf);
}
void ghost(void) {
    scr[gx-1][gy] =' ';
    scr[gx][gy]=' ';
    scr[gx+1][gy] =' ';
    prevgx = gx;
    prevgy = gy;
    if(px < gx & py< gy) {
           if ((scr[prevgx][prevgy-1]=='-' || scr[prevgx][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman || scr[prevgx][prevgy-1]=='.') & (scr[prevgx][prevgy-1]== pacman || scr[prevgx][prevgy-1]=='-' ||  scr[prevgx-1][prevgy-1]==' ' | scr[prevgx-1][prevgy-1]=='.')&(scr[prevgx][prevgy-1]=='-' || scr[prevgx+1][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman|| scr[prevgx+1][prevgy-1]=='.') )
               gy--;
           else if (scr[prevgx-2][prevgy]=='-' || scr[prevgx-2][prevgy]== pacman || scr[prevgx-2][prevgy]==' ' || scr[prevgx-2][prevgy]=='.')
               gx--;
           else if ((scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx][prevgy+1]==' ' || scr[prevgx][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx-1][prevgy+1]==' ' || scr[prevgx-1][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx+1][prevgy+1]==' ' || scr[prevgx+1][prevgy+1]=='.'))
               gy++;
           else if (scr[prevgx+2][prevgy]=='-' || scr[prevgx+2][prevgy]== pacman || scr[prevgx+2][prevgy]==' ' || scr[prevgx+2][prevgy]=='.')
               gx++;}
           else  if(px < gx & py> gy) {
               if ((scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx][prevgy+1]==' ' || scr[prevgx][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx-1][prevgy+1]==' ' || scr[prevgx-1][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx+1][prevgy+1]==' ' || scr[prevgx+1][prevgy+1]=='.'))
                   gy++;
               else if (scr[prevgx-2][prevgy]=='-' || scr[prevgx-2][prevgy]== pacman || scr[prevgx-2][prevgy]==' ' || scr[prevgx-2][prevgy]=='.')
                   gx--;
               else if ((scr[prevgx][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman || scr[prevgx][prevgy-1]=='.') & (scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx][prevgy-1]== pacman | scr[prevgx-1][prevgy-1]==' ' | scr[prevgx-1][prevgy-1]=='.')&(scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx+1][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman|| scr[prevgx+1][prevgy-1]=='.') )
                   gy--;
               else if (scr[prevgx+2][prevgy]=='-' || scr[prevgx+2][prevgy]== pacman || scr[prevgx+2][prevgy]==' ' || scr[prevgx+2][prevgy]=='.')
                   gx++;
           }else  if(px > gx & py> gy) {
               if ((scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx][prevgy+1]==' ' || scr[prevgx][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx-1][prevgy+1]==' ' || scr[prevgx-1][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx+1][prevgy+1]==' ' || scr[prevgx+1][prevgy+1]=='.'))
                   gy++;
               else if (scr[prevgx+2][prevgy]=='-' || scr[prevgx+2][prevgy]== pacman || scr[prevgx+2][prevgy]==' ' || scr[prevgx+2][prevgy]=='.')
                   gx++;
               else if (scr[prevgx-2][prevgy]=='-' || scr[prevgx-2][prevgy]== pacman || scr[prevgx-2][prevgy]==' ' || scr[prevgx-2][prevgy]=='.')
                   gx--;
               else if ((scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman || scr[prevgx][prevgy-1]=='.') & (scr[prevgx+1][prevgy-1]=='-' || scr[prevgx][prevgy-1]== pacman | scr[prevgx-1][prevgy-1]==' ' | scr[prevgx-1][prevgy-1]=='.')&(scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx+1][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman|| scr[prevgx+1][prevgy-1]=='.') )
                   gy--;
           }else if (px > gx & py == gy){
               if (scr[prevgx+2][prevgy]=='-' || scr[prevgx+2][prevgy]== pacman || scr[prevgx+2][prevgy]==' ' || scr[prevgx+2][prevgy]=='.')
                   gx++;
               else if ((scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman || scr[prevgx][prevgy-1]=='.') & (scr[prevgx+1][prevgy-1]=='-' || scr[prevgx][prevgy-1]== pacman | scr[prevgx-1][prevgy-1]==' ' | scr[prevgx-1][prevgy-1]=='.')&(scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx+1][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman|| scr[prevgx+1][prevgy-1]=='.') )
                   gy--;
               else if (scr[prevgx-2][prevgy]=='-' || scr[prevgx-2][prevgy]== pacman || scr[prevgx-2][prevgy]==' ' || scr[prevgx-2][prevgy]=='.')
                   gx--;
               else if ((scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx][prevgy+1]==' ' || scr[prevgx][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx-1][prevgy+1]==' ' || scr[prevgx-1][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx+1][prevgy+1]==' ' || scr[prevgx+1][prevgy+1]=='.'))
                   gy++;
           }else if (px < gx & py == gy){
               if (scr[prevgx-2][prevgy]=='-' || scr[prevgx-2][prevgy]== pacman || scr[prevgx-2][prevgy]==' ' || scr[prevgx-2][prevgy]=='.')
                   gx--;
               else if ((scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx][prevgy+1]==' ' || scr[prevgx][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx-1][prevgy+1]==' ' || scr[prevgx-1][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx+1][prevgy+1]==' ' || scr[prevgx+1][prevgy+1]=='.'))
                   gy++;
               else if (scr[prevgx+2][prevgy]=='-' || scr[prevgx+2][prevgy]== pacman || scr[prevgx+2][prevgy]==' ' || scr[prevgx+2][prevgy]=='.')
                   gx++;
               else if ((scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman || scr[prevgx][prevgy-1]=='.') & (scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx][prevgy-1]== pacman | scr[prevgx-1][prevgy-1]==' ' | scr[prevgx-1][prevgy-1]=='.')&(scr[prevgx+1][prevgy-1]=='-' || scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx+1][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman|| scr[prevgx+1][prevgy-1]=='.') )
                   gy--;
            } else {
               if ((scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman || scr[prevgx][prevgy-1]=='.') & (scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx][prevgy-1]== pacman | scr[prevgx-1][prevgy-1]==' ' | scr[prevgx-1][prevgy-1]=='.')&(scr[prevgx+1][prevgy-1]=='-' ||scr[prevgx+1][prevgy-1]==' ' || scr[prevgx][prevgy-1]== pacman|| scr[prevgx+1][prevgy-1]=='.') )
                   gy--;
               else if (scr[prevgx+2][prevgy]=='-' || scr[prevgx+2][prevgy]== pacman || scr[prevgx+2][prevgy]==' ' || scr[prevgx+2][prevgy]=='.')
                   gx++;
               else if ((scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx][prevgy+1]==' ' || scr[prevgx][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx-1][prevgy+1]==' ' || scr[prevgx-1][prevgy+1]=='.') & (scr[prevgx][prevgy+1]=='-' || scr[prevgx][prevgy+1]== pacman || scr[prevgx+1][prevgy+1]==' ' || scr[prevgx+1][prevgy+1]=='.'))
                   gy++;
               else if (scr[prevgx-2][prevgy]=='-' || scr[prevgx-2][prevgy]== pacman || scr[prevgx-2][prevgy]==' ' || scr[prevgx-2][prevgy]=='.')
                   gx--;
            }
          msg(gx-1, gy,gcol, "*v*");
}

// Interpret a key press and update the data structures.
void update(char in)
{
    ghost();

  switch(in) {
    case 'a':
    case 'h': dx=-1; dy=0;pacman = '<';
    break;
    case 's':
    case 'j': dx=0; dy=1; pacman = 'V';
    break;
    case 'w':
    case 'k': dx=0; dy=-1;pacman = '^';
    break;
    case 'd':
    case 'l': dx=1; dy=0;pacman = '>';
    break;
    default: break;
  }
if (scr[px+dx][py+dy] == '|' || scr[px+dx][py+dy] == '=' || scr[px+dx][py+dy] == ';' || scr[px+dx][py+dy] == '!'){
    dx=0; dy=0;
}
  px += dx;
  py += dy;

    //fruit
if (score == 50 ||  score == 150)
    msg(14,11,0x60,"o");

if (scr[px][py] == 'o') {
    getfruit(); move();
  }

if (scr[px][py] == '.') {
    score++;
  }
if (scr[px][py] == ' ' || scr[px][py] == '.' || scr[px][py] == '-' ||scr[px][py] == pacman) {
       move();
  }
if (scr[px][py] == '*' || scr[px][py] == 'v') {
    collision();
    phase = SPLASH;
    splash_ticks=0;
  }
//if (mp->nexttick >= MAXTICKS)
//        midi_init(shrt);
}

void animate(void)
{
  if (phase == SPLASH) {
    if (splash_ticks < 10) {
      while (available())
        getchar();
      splash_ticks++;
      return;
    }
    // Stall waiting for a key.
    while (!available())
      ;
    getchar();

    // Get the timer counter value for the random seed.
    int seed=get_seed();
    srandom(seed);
    init();
    mp = midi_init(shrt);
    init_TIM3(50);
    render();
    if (mp->nexttick >= MAXTICKS)
          midi_init(shrt);
    clear();
    phase = RUNNING;
  }
  char in=' ';
  if (phase == RUNNING) {
    while(available()) {
      in = getchar();
    }
//    if (in == 'q') {
//#ifdef __linux__
//      cursor_on();
//      cooked_mode();
//      exit(0);
//#else
//      clear();
//      render();
//      splash();
//      phase = SPLASH;
//      splash_ticks = 0;
//#endif
//    }
    if (in == 'p') {
        init_TIM3(50);
        render();
        if (mp->nexttick >= MAXTICKS)
            mp = midi_init(pause);
        freeze();
    }
    update(in);
    render();
    if (mp->nexttick >= MAXTICKS)
          midi_init(shrt);
    return;
  }
}
