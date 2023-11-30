# Data Persistence

In this section we are finally going to start building a legit database. We will implement the logic for writing data to a table and persisting that data to disk. As part of this effort we will begin building a bona-fide storage engine, which needs to be able to read and write files to disk, as well as pull individual data pages into memory.

For the data file interface, we're not going to get too crazy. We just need a mechanism that checks a specific directory for a data file and will create the file if it doesn't exist. Beyond that, we just need to write a couple simple functions that are able to pull specific data pages from disk into memory, as well as write data pages from memory to disk.

The interface for data pages is comparatively more complex. We will need methods for updating each of the header fields, as well as logic to make sure those fields are synchronized with any changes to the data on the page. Moreover, we need methods that can construct a data record from the user-provided input data, and be able to maintain the slot array when inserting data.

But before we do all that fun stuff, we have a boring task to take care of first: a configuration file.

There are a few settings I'll want to be able to easily change down the road, and a config file seems to be the easiest way to do it.

Buckle up, this section is going to be a long one.