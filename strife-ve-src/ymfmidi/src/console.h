#ifndef __CONSOLE_H
#define __CONSOLE_H

void consoleOpen();
void consoleClose();
int consoleGetKey();
void consolePos(short row, short col = 0);

#endif // __CONSOLE_H

