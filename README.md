# Randodo - random strings generator

## What is Randodo?

My goal is to create a simple & fast **random strings generator**, which should be available both as a **command-line utility** and a **C++ library**. Users should be able to control how the generated strings might look like by writing simple specifications, similar to typical **regular expressions**.

Randodo definitely **isn't finished yet**. It should already be quite a useful tool, but lots of optimizations & features which are planned are not done yet (zooming through the code might reveal a few TODOs and alikes); so if you find it useful but lacking a few things, please don't hesitate and tell me about it! This might be the motivation I need to finish it. :-)

## Use cases 

Randodo has been started as a tool useful for testing other stuff, so I would guess that **testing** will be the main use of Randodo. But I wouldn't mind being surprised!

## Examples

Here's a simple example of how you can use Randodo as a command-line utility. At first, you need to create a file with a specification for Randodo:
```
vrok@laptok:~/randodo$ cat sample.rdd 
male=(John|Paul|Martin|Hubert|Bozydar)
female=(Ann|Sharon|Liza|Janina)
names=($male|$female)
verb=(loves|hates|likes|ignores)
how_much=(| very{1,5} much)
result=$male $verb $female$how_much. By the way, here are 5 random letters: [a-zA-Z]{5}.
```

In the file visible above, each line defines a generator (and every such generator has a name). Generators can embed each other to create more powerful generators. Now we can choose one of them to get a few strings - let's say that we'd like to have 3 strings from the last generator in the file, called "result". This is how it can be done:

```
vrok@laptok:~/randodo$ ./randodo sample.rdd result 3
Bozydar likes Janina very much. By the way, here are 5 random letters: sEBOM.
John hates Ann very very much. By the way, here are 5 random letters: uLQMt.
Bozydar likes Sharon. By the way, here are 5 random letters: wDgMR.
```
