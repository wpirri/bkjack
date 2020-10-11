/* carta.h */
#ifndef _CARTA_H_
#define _CARTA_H_

#include <curses.h>

//#include "mesa.h"

class CCarta
{
public:
	CCarta(WINDOW *pwin, int x, int y, int carta, int palo, int visible);
	virtual ~CCarta();
	void Update();
	void Mostrar();
	int Visible();

protected:
	void Palo(int palo, char* crh);
	WINDOW *m_pWindow;
	int X, Y, m_palo, m_carta;
	int m_visible;
};
#endif /*_CARTA_H_*/

