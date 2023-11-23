# BurkeQL - Relational Database From Scratch

BurkeQL is a relational database we are going to write from scratch in C. The current source code can be found [here](https://github.com/burke1791/burkeql). However, please note the code we write as part of this tutorial will be different from the "official" code.

## About Me

My name is Chris Burke. I am a database engineer by day, and a tech nerd by night. I love teaching my coworkers about database internals and thought it would be fun to take it a step further and teach myself how to write the code that runs modern relational databases. 

## Disclaimers

I am not an expert at C. I was able to slap together some working, which is what I present in this tutorial. I did not go out of my way to make sure this code is cross-platform - I wrote it using an Ubuntu-22.04 image running in WSL. Therefore, please be aware that directly copying the code to your system may not work.

## Project Organization

This project will have the following folder structure:

```bash
├── Makefile (calls the Makefile in the src directory)
├── docs (the pages for this blog series)
└── src
    ├── **
    ├── include
    │   └── **
    ├── main.c (entry point to our database program)
    └── Makefile (builds the source code inside the src directory)
```
`**` represents the source code directories. For every folder directly under the `include` directory, there will be a corresponding folder immediately under the `src` directory.