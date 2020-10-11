/* bkjugador.h */

#ifndef _BKJUGADOR_H_
#define _BKJUGADOR_H_

//#include "mesa.h"
//#include "sincro.h"

class CBkJugador
{
public:
	CBkJugador(int key);
	virtual ~CBkJugador();

	int Open();
	int Run();
protected:
    CSincro *m_pSincro;
    CMesa   *m_pMesa;
    int     m_key;

};
#endif /*_BKJUGADOR_H_*/

