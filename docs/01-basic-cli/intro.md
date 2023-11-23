# Basic CLI

Before we begin writing the database internals, we need to create the scaffolding that will allow a user to interact with the database. Our implementation will be extremely basic to start, in fact it will only support three commands: `quit`, `insert`, and `select`.

When we start the CLI program, it will immediately enter an infinite loop that waits for user input. When it receives a command from the user, it will evaluate the command to check its validity then execute it if valid. Although our CLI is basic in functionality, we are going to seemingly over-engineer the input parsing by using two purpose-built tools: flex and bison.

## Flex and Bison

Installation on Ubuntu:
```bash
sudo apt update
sudo apt install flex
sudo apt install bison
```

These two tools are used all over the place to build compilers. In fact, postgres uses them to implement its SQL parser. With flex, you build what's called a lexer - a program that scans input from a buffer and looks for matching patterns. Those matching patterns are chunked together into tokens and passed on to bison.

Bison takes those tokens and attempts to fit them into its grammar - the syntax rules we define. For example, say we pass the following query to a flex/bison SQL parser:

```
Select colname
From tablename;
```

Flex would find five tokens: 'Select', 'colname', 'From', 'tablename', and ';'. It sends those tokens one at a time to bison for syntax analysis. Since this is a valid SQL query, bison will not throw an error.

But what if we tried to send this through a SQL parser:

```
Select
From tablename;
```

Flex would find four tokens this time and send them to bison one at a time. Bison receives the 'Select' token and starts down the rules path for a select statement. Bison is now expecting a list of column names (or a '*'), but it receives the 'From' token, which is a reserved keyword in SQL and not valid in a select column list. Because the syntax is invalid, bison throws an error stating invalid syntax.

I won't be doing a deep dive on flex and bison, as they are very flexible tools that can do a lot more than what we need. Instead, I'm just going to give a high level explanation of our implementation. If you don't care about the lexing and parsing part of the code, then you can skip those sections.

## BQL

As we build out the database system, we'll naturally need a more robust query language than the three basic commands we're starting with. That's where BQL comes in. BQL stands for Burke Query Language, or if you're offput by my vanity, you can call it Basic Query Language.

BQL is going to look a lot like SQL, but it won't be nearly as complex as any of the existing SQL dialects out there. However, we will make it robust enough to do everything we need to do.