# BurkeQL Relational Database

BurkeQL is a relational database we are going to write from scratch in C. The current source code can be found [here](https://github.com/burke1791/burkeql-db).

## Why?

**The dumb name?** Well, my name is Chris Burke and I like to incorporate my last name into a lot of personal projects - call it vanity if you must. Plus, a single syllable is easy to squeeze into made up names.

**This project?** I love databases. I am a database engineer by day and love teaching my app dev coworkers how to make their queries go fast. And I often find an excuse to dive into database internals as part of query tuning exercises.

Learning C has been on my list for a while. As I was getting started, I came across cstack's [Let's Build a Simple Database](https://cstack.github.io/db_tutorial/) tutorial and loved it. He builds a simple sqlite clone that stores data in a B+tree structure and persists it to disk. However, at the time of writing, his sqlite clone only supports a hard-coded table definition.

Going through his tutorial inspired me to start this project with the ultimate goal of having a DBMS that can support any number of arbitrarily defined tables. Though after writing a few sections, I realize my ambitious project is not nearly as approachable as cstack's. If you find your eyes glazing over as you work through my tutorial, I strongly suggest you go through his (currently) 14-part series.

## Disclaimers

I am not an expert at C. I was able to slap together some working code, which is what I present here. I did not go out of my way to make sure the code is cross-platform - I wrote it using an Ubuntu-22.04 image running in WSL. Therefore, please be aware that directly copying the code to your system may not work.. and I am not nearly qualified enough to help you get it working.

## What You Will Build

This tutorial will walk you through my process of building a relational database from scratch. I will provide diffs of every single line of code I add/change/delete, as well as keep an organized Github repository for easy reference. For each of the major sections in this tutorial, there will be a corresponding branch in my Github repo that contains the code as it was at the completion of that section.

This RDBMS will be a command-line program that accepts SQL-like input and behaves as you'd expect a database client to. The final result will be a fully functional, persistent, relational database that's capable of storing all of the basic data types in any number of tables you want.

Check out the [Project Plan](00-intro/project-plan) to see what I currently have planned.
