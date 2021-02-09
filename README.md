# Pacman-with-STM32F0DISCOVERY

The game runs under release mode.
PA9 as TX, PA10 as RX

Player uses 'WASD' to control the direction of the Pacman. The goal is to get as much as score as you can while avoid the chase from the cute blue ghost.
Each pink little dot worth one score, and each big blue circle worth 50 score.
One big blue circle is initiated at the begining of the game. once the player get 50 or 150 score, another one big blue circle appears under the cage of the ghost.

The ghost can erase the little dot, so the player needs to misdirect the ghost to decrease the erasion of dots.

Press 'P' to pause the music and game with a "pause" sound.
Eat blue circle with a "fruit" sound.
Caught up by the ghost with a "death" sound.
Once the player get a new record, the messages will be updated on the screen.
