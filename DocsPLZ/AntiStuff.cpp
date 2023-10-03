#include "commun.h"


BOOL IsDebuggerPresentEx() {

	// getting the PEB structure
	PPEB					pPeb = (PEB*)(__readgsqword(0x60));

	if (pPeb->BeingDebugged == 1)
		return TRUE;

	return FALSE;
}