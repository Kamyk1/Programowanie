#ifndef _SCREEN_H_
#define _SCREEN_H_

void init_screen();
void done_screen();
void gotoyx(int y, int x);
int ngetch();
void getscreensize(int& y, int& x);

#endif /* _SCREEN_H_ */