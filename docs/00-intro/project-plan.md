# BurkeQL - Project Plan

This page contains an outline of my plan for this project. It's meant to serve as a scratchpad for me to organize my thoughts. Please ignore any notes that don't make sense, I probably just forgot to delete/move them.

## Milestones

| Milestone | Status |
| --------- | ------ |
| Basic SQL Parser | Done |
| Data Persistence | In Progress |
| System Catalogs | Planned |
| BQL DML | Planned |

## The Plan

What follows is basically a table of contents for this tutorial/blog series. I list all sections I've completed, as well as future sections I have planned, and any associated notes.

### Basic CLI (COMPLETE)

Writing an extremely basic cli with an extremely basic input parser.

Github branch: [basic-cli](https://github.com/burke1791/burkeql-db/tree/basic-cli)

1. [Intro](../../01-basic-cli/intro)
1. [Flex](../../01-basic-cli/flex)
1. [Bison](../../01-basic-cli/bison)
1. [Putting It Together](../../01-basic-cli/putting-it-together)

### Abstract Syntax Trees (COMPLETE)

Introduce ASTs to the parser.

Github branch: [ast](https://github.com/burke1791/burkeql-db/tree/ast)

1. [Intro](../../02-ast/intro)
1. [AST Interface](../../02-ast/ast-interface)
1. [AST Implementation](../../02-ast/ast-implementation)
1. [Parser Refactor](../../02-ast/parser-refactor)
1. [Parser Interface](../../02-ast/parser-interface)
1. [Putting It Together](../../02-ast/putting-it-together)

### The Database Page (COMPLETE)

This section is entirely conceptual - there will be zero code written.

Discussion about how data are organized on disk. Also touch on how different DBMS's do things slightly different from one another.

1. [Page Structure](../../03-db-page/page-structure)

### Data Persistence (in progress)

Start storing data on disk. Using a hard-coded table definition:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Char(20) Not Null,
    last_name Char(20) Not Null
);
```

- Every insert is immediately persisted to disk
- Talk about the buffer pool? Probably not
- Only a single data page allowed - throw error if page is full
- Page splits implemented in a later section
- Select statement also deferred - will be inspecting data pages using `xxd`

---

1. [Config File](../../04-data-persistence/config-file)
1. [DB File Interface](../../04-data-persistence/file-interface)
1. [Loading Config](../../04-data-persistence/loading-config)
1. DB Page Interface
1. DB Page Implementation
1. Update Parser - Insert
1. Insert Implementation
1. Putting It Together

### Selecting Data (planned)

- Updating the parser to support selecting data from the table
- MAYBE introduce the analyzer
- Implement basic table scan
- RecordDescriptor
- Datum construct

---

1. Update Parser - Select
1. Basic Analyzer (MAYBE)
1. Table Scan
1. Putting It Together

### Page Splits (planned)

- Create a new page when an insert won't fit on an existing page
- Page metadata functionality - linked list in the header fields

### Data Type - Ints (planned)

- Write functions to support all of the `Int` class data types:
    - TinyInt
    - SmallInt
    - Int
    - BigInt

Use a new hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    age_seconds BigInt Not Null,
    age_days SmallInt Not Null,
    age_years TinyInt Not Null
);
```

### Data Type - Bool (planned)

- 1-byte
- Essentially a TinyInt, but can only be 1 or 0 so requires extra code

Hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    is_alive Bool Not Null
);
```

### Data Type - Varchar (planned)

- 2-byte variable offset array
- talk about how column order in the create statement can differ from how the engine stores columns on disk

Hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Varchar(20) Not Null,
    last_name Varchar(50) Not Null,
    age Int Not Null
);
```

### Data Constraint - NULLs (planned)

- Introduce the Null bitmap and explain why it's important
- Implement the Null bitmap

Hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Varchar(20) Null,
    last_name Varchar(50) Not Null
);
```

### System Catalog - _tables (planned)

- Introduce the concept of the system catalog
- Global values
    - `object_id`
    - `page_id`?
- Implement the `_tables` system catalog
- Check for the existence of the system catalogs on boot
- Populate the `_tables` table on startup of a new database

### System Catalog - _columns (planned)

- Same as above, but for columns
- Refactor the retrieval code to use system catalogs for grabbing RecordDescriptors

### BQL - Select From (planned)

### BQL - Create Table (planned)

### BQL - Insert Into (planned)

### BQL - Update (planned)

### BQL - Delete (planned)