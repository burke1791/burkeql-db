# Buffer Pool

Before we go too far in implementing a simple query engine, we need to refactor how our program is able to interact with pages on disk. Right now, we only allow a single data page, so it's easy to pull it into memory at the beginning of the program, pass it around to any systems that need it, then flush to disk at the end.

This won't work when we support multiple pages, and by then we'll have written a lot more code that works with pages so refactoring is best done early. We're going to create a mini buffer pool.

## What is a Buffer Pool?

Relational databases are built to store and work with huge amounts of data. In such situations, the amound of data they store will far exceed the amount of memory they're given by the host machine. So in order to read and write to a lot of data pages while working within its memory constraints, they keep data pages in something called a buffer pool.

A buffer pool is a shared pool of memory that keeps track of all data pages currently in memory. If some process needs to read or write from a specific data page, it will ask the buffer pool for the page. If the page is currently in memory, the buffer pool returns a pointer to the data page quick and easy. If the page is not currently in memory, the buffer pool must first determine if it has enough space to house a new page. If there is enough space, then it'll pull the page from disk into memory and return a pointer to the page. If the buffer pool does not have enough space, it need to choose a page to evict from the pool before it can read the requested page into memory.

At present, our needs are not that complex, so we can get away with writing a really dumb buffer pool. But it is a solid starting point, and a good way to keep data file/page access constrained to a single subsystem.

