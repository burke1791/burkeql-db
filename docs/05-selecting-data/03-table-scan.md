# Table Scan



## Notes

- What information do we need to perform a table scan?
    - TableDescriptor - has a `RecordDescriptor` property for the table's columns
    - List of `ResTarget`s representing which columns to pull from the table
- Resultset structs
- The scan will populate a `Datum` array with the results

