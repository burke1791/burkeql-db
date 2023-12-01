# Config File

There are a handful of configuration settings I'll want to play with in the future, and putting them in a config file is much easier than stashing them in some buried header file. For now, our config file is only going to have one parameter: `DATA_FILE`. It stores the absolute path to the database's data file. Here is what the config file looks like:

`burkeql.conf`

```conf
# Config file for the BurkeQL database

# Absolute location of the data file
DATA_FILE=/path/to/data/file
```

Make sure to set this path to whatever makes sense on your machine. For me, I plan to have all database files in a folder named `db_files` at the root of this repository. I want to keep them local to my code because we're going to be inspecting the binary contents of the file and having them local just makes it easier. For example, my config file looks like this:

```conf
DATA_FILE=/home/burke/source_control/burkeql-db/db_files/main.dbd
```

Note the `.dbd` file extension. This stands for "database data." In the future, we'll introduce a transaction log file that has the extension `.dbl`: "database log." You can use whatever extension you like, or no extension at all - it doesn't matter.

Make sure you name the config file `burkeql.conf` and put it at the root level of your repository. For reference, this is what my repo structure looks like:

```shell
├── burkeql.conf     <--- Config file at root level of the repo
├── db_files
└── src
    ├── Makefile
    ├── include
    │   ├── parser
    │   │   ├── parse.h
    │   │   └── parsetree.h
    ├── main.c
    ├── parser
        ├── gram.y
        ├── parse.c
        ├── parsetree.c
        └── scan.l
```