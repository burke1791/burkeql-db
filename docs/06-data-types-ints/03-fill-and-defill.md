# Fill and Defill

In this section, we're going to update our code to actually insert our new data, and we'll finally be able to test it out by running some insert statements.

Let's pretend we're an insert statement being shuffled along our program's insert code path and make the necessary changes as we find them. Starting at the top, our main function requests input from the user and sends it through the parser. Processing the parse tree is where we come in:

`src/main.c`

```diff
 int main(int argc, char** argv) {
 
 *** code omitted for brevity
 
         break;
       case T_InsertStmt: {
         int32_t person_id = ((InsertStmt*)n)->personId;
         char* name = ((InsertStmt*)n)->name;
+        uint8_t age = ((InsertStmt*)n)->age;
+        int16_t dailySteps = ((InsertStmt*)n)->dailySteps;
+        int64_t distanceFromHome = ((InsertStmt*)n)->distanceFromHome;
-        if (!insert_record(bp, person_id, name)) {
+        if (!insert_record(bp, person_id, name, age, dailySteps, distanceFromHome)) {
           printf("Unable to insert record\n");
         }
         break;
       }
       case T_SelectStmt:
 
 *** code omitted for brevity ***
 
 }
```

We need to extract the new fields from our `InsertStmt` node and pass them to the `insert_record` function. Next we follow the insert path until we get to the `fill_record` and `fill_val` functions.

`src/storage/record.c`

```diff
 static void fill_val(Column* col, char** dataP, Datum datum) {
   int16_t dataLen;
   char* data = *dataP;
 
   switch (col->dataType) {
+    case DT_TINYINT:
+      dataLen = 1;
+      uint8_t valTinyInt = datumGetUInt8(datum);
+      memcpy(data, &valTinyInt, dataLen);
+      break;
+    case DT_SMALLINT:
+      dataLen = 2;
+      int16_t valSmallInt = datumGetInt16(datum);
+      memcpy(data, &valSmallInt, dataLen);
+      break;
     case DT_INT:
       dataLen = 4;
       int32_t valInt = datumGetInt32(datum);
       memcpy(data, &valInt, dataLen);
       break;
+    case DT_BIGINT:
+      dataLen = 8;
+      int64_t valBigInt = datumGetInt64(datum);
+      memcpy(data, &valBigInt, dataLen);
+      break;
     case DT_CHAR:
       dataLen = col->len;
       char* str = strdup(datumGetString(datum));
       int charLen = strlen(str);
       if (charLen > dataLen) charLen = dataLen;
       memcpy(data, str, charLen);
       free(str);
       break;
   }
 
   data += dataLen;
   *dataP = data;
 }
```

We're simply adding more cases to the switch statement to account for the new data types. Make sure you set the `dataLen` to the correct byte size for the data type.

## Datum Conversions

Since we've made several calls to the datum conversion functions for the new data types, we actually need to write them now.

`src/include/storage/datum.h`

```diff
+Datum uint8GetDatum(uint8_t i);
+Datum int16GetDatum(int16_t i);
 Datum int32GetDatum(int32_t i);
+Datum int64GetDatum(int64_t i);
 Datum charGetDatum(char* c);
 
+uint8_t datumGetUInt8(Datum d);
+int16_t datumGetInt16(Datum d);
 int32_t datumGetInt32(Datum d);
+int64_t datumGetInt64(Datum d);
 char* datumGetString(Datum d);
```

`src/storage/datum.c`

```diff
+Datum uint8GetDatum(uint8_t i) {
+  return (Datum) i;
+}
+
+Datum int16GetDatum(int16_t i) {
+  return (Datum) i;
+}
 
 Datum int32GetDatum(int32_t i) {
   return (Datum) i;
 }
 
+Datum int64GetDatum(int64_t i) {
+  return (Datum) i;
+}
 
 Datum charGetDatum(char* c) {
   return (Datum) c;
 }
 
 
+uint8_t datumGetUInt8(Datum d) {
+  return (uint8_t) d;
+}
+
+int16_t datumGetInt16(Datum d) {
+  return (int16_t) d;
+}
 
 int32_t datumGetInt32(Datum d) {
   return (int32_t) d;
 }
 
+int64_t datumGetInt64(Datum d) {
+  return (int64_t) d;
+}
 
 char* datumGetString(Datum d) {
   return (char*) d;
 }
```