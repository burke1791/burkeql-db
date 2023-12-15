#ifndef DATUM_H
#define DATUM_H

#include <stdint.h>

typedef unsigned long Datum;

Datum int32GetDatum(int32_t i);
Datum charGetDatum(char* c);

int32_t datumGetInt32(Datum d);
char* datumGetString(Datum d);

#endif /* DATUM_H */