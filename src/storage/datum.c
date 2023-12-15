#include <stdint.h>

#include "storage/datum.h"

Datum int32GetDatum(int32_t i) {
  return (Datum) i;
}

Datum charGetDatum(char* c) {
  return (Datum) c;
}


int32_t datumGetInt32(Datum d) {
  return (int32_t) d;
}

char* datumGetString(Datum d) {
  return (char*) d;
}