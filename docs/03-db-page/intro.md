# The Database Page

In this section I am going to introduce and discuss the concept of a database "page". I won't be writing any code, so if that's all you're interested in, I suggest you skip to the next section.

## Data File

Databases store data in files on disk. They aren't doing any magic voodoo behind the scenes (although they're so clever it kind of seems like magic), it's just reading and writing data to regular files. Some of the magic comes in to play with how the storage engine organizes data in those files.

Not all RDBMS platforms implement storage in the same way. Some (e.g. postgres) use hundreds of different files - you create a new table, you get a new set of files just for that table. Others (e.g. Microsoft SQL Server) use far fewer files - in its default setup, you get two files. One for the data and one for the transaction log.

The system we're building is going to be similar to MS SQL Server - one file for the data, and one for the transaction log.. if I ever get around to implementing it.

## Data Page

Within the data files, the storage engine organizes the data into uniformly-sized chunks - typically 8kB. The is the most basic unit that database systems work with. If you ask the DB to return a single row from a table, it first has to read an entire 8kB page into memory, then it can scan the page for the record you requested.

So we have 8kB pages, how are they structured?

### Page Header

You can think of a data page as being composed of three sections. The first is the page header, which contains metadata about itself. The size and type of information stored in a page header differ from system to system. In our system, we will be working with pages that have 20-byte headers.

The header fields are as follows:

| Header Field | Size | Description |
| pageId | 4-bytes | One-based page identifier. Used to mark the byte position of the page in the file. |
| pageType | 1-byte | Identifies the type of page. 0 = data page, 1 = index page |
| indexLevel | 1-byte | Level of the page in an index. 0 = leaf level (or heap page) |
| prevPageId | 4-bytes |