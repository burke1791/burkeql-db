# The Database Page

In this section I am going to introduce and discuss the concept of a database "page". I won't be writing any code, so if that's all you're interested in, I suggest you skip to the next section.

## Data File

Databases store data in files on disk. They aren't doing any magic voodoo behind the scenes (although they're so clever it kind of seems like magic), it's just reading and writing data to regular files. Some of the magic comes in to play with how the storage engine organizes data in those files.

Not all RDBMS platforms implement storage in the same way. Some (e.g. postgres) use hundreds of different files - you create a new table, you get a new set of files just for that table. Others (e.g. Microsoft SQL Server) use far fewer files - in its default setup, you get two files. One for the data and one for the transaction log.

The system we're building is going to be similar to MS SQL Server - one file for the data, and one for the transaction log.. if I ever get around to implementing it. Moreover, the structure of our data pages will not align with any existing RDBMS out there. Instead I'm using a combination of MS SQL Server and Postgres - taking the things I like about both and ignoring what I don't like.

## Data Page

Within the data files, the storage engine organizes the data into uniformly-sized chunks - typically 8kB. The is the most basic unit that database systems work with. If you ask the DB to return a single row from a table, it first has to read an entire 8kB page into memory, then it can scan the page for the record you requested.

So we have 8kB pages, how are they structured?

![Data Page Structure](assets/data_page.png)

### Page Header

You can think of a data page as being composed of three sections. The first is the page header, which contains metadata about itself. The size and type of information stored in a page header differ from system to system. In our system, we will be working with pages that have 20-byte headers.

The header fields are as follows:

| Header Field | Size | Description |
| ------------ | ---- | ----------- |
| pageId | 4-bytes | One-based page identifier. Used to mark the byte position of the page in the file. |
| pageType | 1-byte | Identifies the type of page. 0 = data page, 1 = index page |
| indexLevel | 1-byte | Level of the page in an index. 0 = leaf level (or heap page) |
| prevPageId | 4-bytes | Points to the previous page at the same `indexLevel`. If there is no previous page, then its value is 0. |
| nextPageId | 4-bytes | Points to the next page at the same `indexLevel`. If there is no next page, then its value is 0. |
| numRecords | 2-bytes | Number of records stored on the page |
| freeBytes | 2-bytes | Number of unclaimed bytes on the page |
| freeData | 2-bytes | Number of continuous free bytes between the end of the last record and the beginning of the slot array |

**pageId**

The `pageId`'s purpose is to tell the storage engine where it belongs within the data file. In a database with 8kB pages, we know that `pageId=1` belongs at the beginning of the file. `pageId=3` belongs at byte offset (3-1) * 8192 = 16,384 in the file.

The first thing that probably popped out at you is the `pageId` being a one-based identifier instead of zero-based. That's because we need to reserve zero as "nothing" for the `prevPageId` and `nextPageId` header fields. The "nothing" indicating there is not prev or next page.

**pageType and indexLevel**

These fields will be important when we implement indexes. But it does serve an important purpose for the query engine. The data stored on index pages looks a lot different than the data stored on data pages, so the query engine needs to know which type of data it's dealing with.

**prevPageId and nextPageId**

Database pages exist as part of a doubly-linked list, and these two fields serve as the pointers in the list. Since we're using a single file for the data in our database, we have to store the data for multiple different tables in the same file. This means the data for `tableA` will not be a contiguous chunk of the file - it will have pages from `tableB` interspersed. These prev and next pointers help the query engine efficiently scan the file for the data it cares about.

**numRecords**

Exactly as the name suggests, it stores the number of records on the page. This field won't be very useful in the near term, but it will help us later on when we decide to store statistics about the tables in our database.

**freeBytes and freeData**

These two will often have the same value, but they serve two different roles. The former is used to determine if the storage engine can reorganize and compact the existing data records to make room for a new one. The latter is used to determine if a new record can fit on the page or not.

In a typical transactional database, we have lots of inserts, updates, and deletes happening all the time. Let's say we have a data page that's completely full and a user wants to delete a record in the middle of the page. The storage engine will mark it as deleted, but it WILL NOT reorganize the existing records towards the beginning of the page. Instead, there will be a hole of empty space in the middle.

Because the empty space is in the middle of the page, when a new insert comes along the storage engine will still consider this page full because the value stored in `freeData` is not large enough. However, `freeBytes` IS large enough, so the storage engine might decide now is a good time to compact the existing records on the page in order to make room at the end of the page for the new one.

### Slot Array

The slot array is responsible for maintaining the order of records stored on the page. This is not as significant in a heap as it is in an index; however, it's important to understand that the records themselves ARE NOT physically stored in any particular order.

The slot array grows from the end of the page towards the beginning. For each new record inserted on the page, the data itself is **appended** to the end of the last record on the page, and a new slot array item is **prepended** to the beginning of the slot array. This means in order for a page to have enough space for a new record, it must have enough empty space for the record itself AND space for the new slot array entry.

Each slot array item takes up 4-bytes. In order to calculate how many bytes the whole slot array consumes, you can multiply the `numRecords` header field by 4.

### Record Structure

