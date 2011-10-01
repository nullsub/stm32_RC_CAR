#include "common.h"

void itoa(int z, char *Buffer, int base_NOT_USED_ALWAYS_TEN)
{
	int i = 0;
	int j;
	char tmp;
	unsigned u;		// In u bearbeiten wir den Absolutbetrag von z.

	// ist die Zahl negativ?
	// gleich mal ein - hinterlassen und die Zahl positiv machen
	if (z < 0) {
		Buffer[0] = '-';
		Buffer++;
		// -INT_MIN ist idR. größer als INT_MAX und nicht mehr 
		// als int darstellbar! Man muss daher bei der Bildung 
		// des Absolutbetrages aufpassen.
		u = ((unsigned)-(z + 1)) + 1;
	} else {
		u = (unsigned)z;
	}
	// die einzelnen Stellen der Zahl berechnen
	do {
		Buffer[i++] = '0' + u % 10;
		u /= 10;
	} while (u > 0);

	// den String in sich spiegeln
	for (j = 0; j < i / 2; ++j) {
		tmp = Buffer[j];
		Buffer[j] = Buffer[i - j - 1];
		Buffer[i - j - 1] = tmp;
	}
	Buffer[i] = '\0';
}

int atoi(char *c)
{
	int res = 0;
	while (*c >= '0' && *c <= '9')
		res = res * 10 + *c++ - '0';
	return res;
}

