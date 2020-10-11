/* *****************************************************************************
*	carta.C
*
***************************************************************************** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "bkjack.h"

//#include "status.h"
#include "carta.h"

extern void ShowMessage(const char* msg);

CCarta::CCarta(WINDOW* pwin, int x, int y, int carta, int palo, int visible)
{
	char strpalo[4];

	m_pWindow = pwin;
	X = x;
	Y = y;
	m_palo = palo;
	m_carta = carta;
	m_visible = visible;
	if(visible)
	{
		Palo(m_palo, strpalo);
		mvwprintw(pwin,y,x,"%2i %s", m_carta, strpalo);
	}
	else
	{
		mvwprintw(m_pWindow,Y,X,"?? ???");
	}
	wrefresh(m_pWindow);
}

CCarta::~CCarta()
{
	mvwprintw(m_pWindow,Y,X,"      ");
}

void CCarta::Update()
{
    wclear(stdscr);
    touchwin(stdscr);
    touchwin(m_pWindow);
	wnoutrefresh(m_pWindow);
	doupdate();
}

void CCarta::Mostrar()
{
	char strpalo[4];

	Palo(m_palo, strpalo);
	mvwprintw(m_pWindow,Y,X,"%2i %s", m_carta, strpalo);
	wrefresh(m_pWindow);
}

int CCarta::Visible()
{
	return m_visible;
}

void CCarta::Palo(int palo, char* chr)
{
	switch(palo)
	{
		case 1:
			strcpy(chr, "cor");
			break;
		case 2:
			strcpy(chr, "tre");
			break;
		case 3:
			strcpy(chr, "pic");
			break;
		case 4:
			strcpy(chr, "dia");
			break;
		default:
			strcpy(chr, "---");
			break;
	}
}

