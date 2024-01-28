# Temporary Code Refactor

This is the last time we'll have to refactor code that we know is going to be thrown out in the future. Hurray! But I must begin with an apology. The amount of refactoring we need to do is quite significant, and it's made worse knowing we're going to toss it all out when we implement system catalogs soon.

But this is the last time we'll have to write a bunch of throwaway code, so that's something!

You're probably wondering why the refactor will be so large since our table is pretty much the same. In order to support `Null`'s, we need to restructure our parser's insert statement grammar, which means we need to redo the `InsertStmt` struct, which means all of our temporary code that works with that struct needs to change as well. It's a real house of cards/trail of dominos situation.

Let's just dive right in, starting with the parser.

## Parser Refactor

As a reminder, our table DDL looks like this:

```sql
Create Table person (
    person_id Int Not Null,
    first_name Varchar(20) Null,
    last_name Varchar(20) Not Null,
    age Int Null
);
```

We have two columns that can store `Null` values. And in order to consume `Null`'s from the client, we need to update our scanner and grammar to be able to parse them. The scanner is quite simple:

`src/parser/scan.l`

```diff
 INSERT    { return INSERT; }
 
+NULL      { return KW_NULL; }
 
 SELECT    { return SELECT; }
```

We just add a new "NULL" keyword.

