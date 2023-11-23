# Write Your Own Relational Database

The following series of blog posts is a cumulative guide on how to write a relational database engine from scratch in C.

## Basic CLI
1. Infinite loop asking for user commands
    - \q
    - insert (not implemented yet)
    - select (not implemented yet)
1. Introduce DB Pages
    - Page header
    - Slot array
    - Record structure
1. Insert command with hard-coded table definition
    - Table with the following definition:
    ```
    Create Table person (
        person_id Int Not Null,
        first_name Char(20) Not Null,
        last_name Char(20) Not Null
    )
    ```
    - Insert command with space-delimited column values, e.g.
    `i [person_id] [first_name] [last_name]`
    - Page splits not allowed
1. Select command
    - Selects all rows and all columns
    - Helper code to format the console output
1. Persist pages to disk
    - On startup, user must supply a DB file. If it doesn't exist, we'll create it.
    - Every insert is immediately persisted to disk
1. Page splits
    - Allow new pages to be created when an insert won't fit in any existing pages
1. Boot Sequence
    - Introduce the concept of a boot sequence
    - If starting up a brand new database, it will create the necessary data before handing control back to the user
    - If loading an existing database, it will make sure the boot sequence completed successfully, then hand control back to the user
1. System catalogs: _tables
    - Add the _tables system catalog to the boot sequence
    - Refactor the retrieval code to use the system catalog for table scanning
1. System catalogs: _columns
    - Add the _columns system catalog to the boot sequence
    - Refactor the retrieval code

## BQL - A Simple SQL Dialect

## Analyzer

## Planner

## Executor

## Write-Ahead Logging

## Concurrency Control

## 
