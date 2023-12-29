#ifndef DATUM_H
#define DATUM_H

#include <stdint.h>

typedef unsigned long Datum;

Datum uint8GetDatum(uint8_t i);
Datum int16GetDatum(int16_t i);
Datum int32GetDatum(int32_t i);
Datum int64GetDatum(int64_t i);
Datum charGetDatum(char* c);

uint8_t datumGetUInt8(Datum d);
int16_t datumGetInt16(Datum d);
int32_t datumGetInt32(Datum d);
int64_t datumGetInt64(Datum d);
char* datumGetString(Datum d);

#endif /* DATUM_H */