# Table Scan



## Notes

- What information do we need to perform a table scan?
    - TableDescriptor - has a `RecordDescriptor` property for the table's columns
    - A `ResultSet` struct to dump the values into. The `ResultSet` has a `RecordDescriptor` property that only contains the columns that we want from the table. This will be built by the analyzer in our mature system.
- The scan will populate a `LinkedList` as it pulls records from the table
- Only the columns found in the `ResTarget` array will end up in the `ResultSet`

- How does the table scan work?
    - Start at the first `pageId` of the table
    - Loop through the slot array and extract the columns we care about.
    - Put these column values in a `Datum` array
    - Append the `Datum` array to the `RecordSet->rows` linked list

