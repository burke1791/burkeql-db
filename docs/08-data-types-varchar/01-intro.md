# Varchar Intro

The last data type we're going to add (for now) to our database system is the variable-length `Varchar` type. Its purpose is to store text data, just like the `Char` type, but unlike the `Char`, our storage engine will only store as many bytes required to represent the data. For example, consider these two tables:

```sql
Create Table person_char (
    person_id Not Null,
    first_name Char(20) Not Null,
    last_name Char(20) Not Null
);

Create Table person_varchar (
    person_id Not Null,
    first_name Varchar(20) Not Null,
    last_name Varchar(20) Not Null
);
```

Let's say we want to store the following values in each table. How many bytes for the data do records in each table require (ignoring overhead)?

| person_id | first_name | last_name |
|-----------|------------|-----------|
| 1 | Chris | Burke |

Let's compare...

| column | person_char | person_varchar |
|--------|-------------|----------------|
| person_id | 4-bytes | 4-bytes |
| first_name | 20-bytes | 5-bytes |
| last_name | 20-bytes | 5-bytes |
| **TOTAL** | 44-bytes | 14-bytes |

As you can see, `Varchar` columns have a humongous advantage over `Char` columns simply because they can adapt to the size of the data stored in them. `Char`'s are so archaic that if you're ever designing a data model, you should pretend `Char` doesn't exist - there's no reason to choose them over `Varchar` (or `Text` in postgres).

While `Varchar`'s do offer a big storage savings over `Char`'s, it's important to note there is some overhead associated with storing `Varchar` data. Namely, we must store its length so that our code knows exactly where to stop reading from the data page.

Different database systems implement the storage mechanism for variable-length columns differently, i.e. Postgres does it different from MS SQL Server does it different from MySQL. Our implementation will be somewhat similar to MS SQL Server.

The unique thing about SQL Server is the storage engine will group all fixed-length columns at the beginning of the record, and all variable-length columns at the end. It doesn't matter what order we list the columns in our `Create Table` statement, the engine always places the variable-length columns at the end. Then we also store a 2-byte field prepended to the `Varchar` data that has the length of the data, including the 2-byte overhead. So if we revisit the `Char` vs `Varchar` comparison from above, but this time include the `Varchar` overhead, we'll get:

| column | person_char | person_varchar |
|--------|-------------|----------------|
| person_id | 4-bytes | 4-bytes |
| first_name | 20-bytes | 7-bytes |
| last_name | 20-bytes | 7-bytes |
| **TOTAL** | 44-bytes | 18-bytes |

We need two additional bytes for each of the `Varchar` columns.

Alright enough yappin', let's get to the code.