# BurkeQL - Project Roadmap

This page contains an outline of my plan for this project. It's really meant to serve as a scratchpad for me to organize my thoughts. Please ignore any notes that don't make sense, I probably just forgot to delete/move them.

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

### Data Persistence (COMPLETE)

Start storing data on disk. Using a hard-coded table definition:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null
);
```

- Every insert is immediately persisted to disk
- Talk about the buffer pool? Probably not
- Only a single data page allowed - throw error if page is full
- Page splits implemented in a later section
- Select statement also deferred - will be inspecting data pages using `xxd`

---

1. [Intro](../../04-data-persistence/01-intro)
1. [Config File](../../04-data-persistence/02-config-file)
1. [Loading Config](../../04-data-persistence/03-loading-config)
1. [DB File Interface](../../04-data-persistence/04-file-interface)
1. [DB Page Interface](../../04-data-persistence/05-page-interface)
1. [DB Page Implementation](../../04-data-persistence/06-page-implementation)
1. [Parser Refactor - Insert](../../04-data-persistence/07-parser-refactor-insert)
1. [Serializing and Inserting Data](../../04-data-persistence/08-serializing-and-inserting-data)

### Selecting Data (COMPLETE)

- Updating the parser to support selecting data from the table
- MAYBE introduce the analyzer
- Implement basic table scan
- RecordDescriptor
- Datum construct

---

1. [Parser Refactor - Select](../../05-selecting-data/01-parser-refactor-select)
1. [The SQL Analyzer](../../05-selecting-data/02-the-sql-analyzer)
1. [Buffer Pool](../../05-selecting-data/03-buffer-pool) 
1. [Table Scan](../../05-selecting-data/04-table-scan)
1. [Displaying Results](../../05-selecting-data/05-displaying-results)

### Data Type - Ints (COMPLETE)

- Write functions to support all of the `Int` class data types:
    - TinyInt
    - SmallInt
    - Int
    - BigInt

Use a new hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null,
    age TinyInt Not Null,
    daily_steps SmallInt Not Null,
    distance_from_home BigInt Not Null
);
```

1. [Hard-Coded Table Refactor](../../06-data-types-ints/01-hard-coded-table-refactor)
1. [Parser Refactor](../../06-data-types-ints/02-parser-refactor)
1. [Fill and Defill](../../06-data-types-ints/03-fill-and-defill)
1. [Select Updates](../../06-data-types-ints/04-select-updates)

### Data Type - Bool (COMPLETE)

- 1-byte
- Essentially a TinyInt, but can only be 1 or 0 so requires extra code

Hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    name Char(20) Not Null,
    age TinyInt Not Null,
    daily_steps SmallInt Not Null,
    distance_from_home BigInt Not Null,
    is_alive Bool Not Null
);
```

1. [Hard-Coded Table Refactor](../../07-data-types-bool/01-hard-coded-table-refactor)
1. [Parser Refactor](../../07-data-types-bool/02-parser-refactor)
1. [Fill and Defill](../../07-data-types-bool/03-fill-and-defill)
1. [Select Updates](../../07-data-types-bool/04-select-updates)

### Data Type - Varchar (COMPLETE)

- 2-byte variable length overhead
- talk about how column order in the create statement can differ from how the engine stores columns on disk

Hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Varchar(20) Not Null,
    last_name Varchar(20) Not Null,
    age Int Not Null
);
```

1. [Intro](../../08-data-types-varchar/01-intro)
1. [Storage and Fill](../../08-data-types-varchar/02-storage-and-fill)
1. [Temporary Code Refactor](../../08-data-types-varchar/03-temporary-code-refactor)
1. [Inserting Data](../../08-data-types-varchar/04-inserting-data)
1. [Defill and Display](../../08-data-types-varchar/05-defill-and-display)

### Data Constraint - NULLs (in progress)

- Introduce the Null bitmap and explain why it's important
- Implement the Null bitmap

Hard-coded table:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Varchar(20) Null,
    last_name Varchar(20) Not Null,
    age Int Null
);
```

1. [Intro](../../09-data-constraint-null/01-intro)
1. [Parser Refactor](../../09-data-constraint-null/02-parser-refactor)
1. [Writing Data](../../09-data-constraint-null/03-writing-data)
1. [Reading Data](../../09-data-constraint-null/04-reading-data)

### Page Splits (planned)

- Create a new page when an insert won't fit on an existing page
- Page metadata functionality - linked list in the header fields
- Because tables are heaps, we will loop through all existing pages until we find one that has enough space
- Introduce buffer tags and fully flesh out the buffer pool (except for locks)

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

### Analyzer (planned)

### BQL - Select From (planned)

### BQL - Create Table (planned)

### BQL - Insert Into (planned)

### BQL - Update (planned)

### BQL - Delete (planned)