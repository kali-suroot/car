#ifndef __MENU_H
#define __MENU_H

#include "sys.h"

extern uint8_t mode_selection;
extern uint8_t displayFlag;

void Mode_selection(void);
void Display(void);
void Automatic(void);
void Anti_theft_mode(void);
void Action(void);

#endif
