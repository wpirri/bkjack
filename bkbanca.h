/* bkbanca.h */

#ifndef _BKBANCA_H_
#define _BKBANCA_H_

//#include "mesa.h"
//#include "sincro.h"

class CBkBanca
{
public:
	CBkBanca(int key);
	virtual ~CBkBanca();

	int Open();
	int Run();

protected:
	CSincro *m_pSincro;
	CMesa	*m_pMesa;
	int		m_key;

};
#endif /*_BKBANCA_H_*/

