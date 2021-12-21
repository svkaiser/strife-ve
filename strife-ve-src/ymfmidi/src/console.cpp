#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>

static HANDLE g_console;
#else
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

static termios g_attr;
#endif

#include "console.h"

// ----------------------------------------------------------------------------
void consoleOpen()
{
#ifdef _WIN32
	// weird hack to use a console buffer in msys2 bash
	if (!isatty(fileno(stdout)))
	{
		FreeConsole();
		AllocConsole();
	}
	
	g_console = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(g_console);

	int fd = _open_osfhandle((intptr_t)g_console, O_WRONLY | O_TEXT);
	dup2(fd, STDOUT_FILENO);
	close(fd);

#else
	termios attr;
	
	tcgetattr(0, &attr);
	g_attr = attr;
	attr.c_lflag &= ~( ICANON | ECHO );
	attr.c_cc[VMIN] = 1;
	attr.c_cc[VTIME] = 0;
	tcsetattr(0, TCSAFLUSH, &attr);

	printf("\x1b[2J");
#endif

	atexit(consoleClose);
}

// ----------------------------------------------------------------------------
void consoleClose()
{
#ifdef _WIN32
	CloseHandle(g_console);
	g_console = nullptr;
#else
	tcsetattr(STDIN_FILENO, TCSANOW, &g_attr);
#endif
}

// ----------------------------------------------------------------------------
int consoleGetKey()
{
	int key = -1;

#ifdef _WIN32
	while (kbhit())
	{
		key = getch();
		if (key == 0xe0)
			key = -getch();
	}
	
	if      (key == -'H') key = -'A'; // cursor up
	else if (key == -'P') key = -'B'; // cursor down
	else if (key == -'M') key = -'C'; // cursor right
	else if (key == -'K') key = -'D'; // cursor left
#else
	static timeval timeout = {0};
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);
	select(1, &readfds, nullptr, nullptr, &timeout);

	while (select(1, &readfds, nullptr, nullptr, &timeout) > 0
	       && FD_ISSET(0, &readfds))
	{
		key = getc(stdin);
		if (key == 0x1b)
		{
			getc(stdin);
			key = -getc(stdin);
		}
	}
#endif

	return key;
}

// ----------------------------------------------------------------------------
void consolePos(short row, short col)
{
#ifdef _WIN32
	const COORD c = {col, row};
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
#else
	printf("\x1b[%d;%dH", row + 1, col + 1);
#endif
}
