#include <stdint.h>

#include "storage/datum.h"

Datum uint8GetDatum(uint8_t i) {
  return (Datum) i;
}

Datum int16GetDatum(int16_t i) {
  return (Datum) i;
}

Datum int32GetDatum(int32_t i) {
  return (Datum) i;
}

Datum int64GetDatum(int64_t i) {
  return (Datum) i;
}

Datum charGetDatum(char* c) {
  return (Datum) c;
}


uint8_t datumGetUInt8(Datum d) {
  return (uint8_t) d;
}

int16_t datumGetInt16(Datum d) {
  return (int16_t) d;
}

int32_t datumGetInt32(Datum d) {
  return (int32_t) d;
}

int64_t datumGetInt64(Datum d) {
  return (int64_t) d;
}

char* datumGetString(Datum d) {
  return (char*) d;
}