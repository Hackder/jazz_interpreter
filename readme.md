# Compilation
```
cmake -S . -B build
cd build
cmake --build . -j 10
```

Two binaries will be generated, one test binary `jazz_test` which will run
the unit tests, and the other `jazz` which is the main program.

To compile with the fuzzing binaries you have to use Clang with libfuzzer included.
If you can't do that, you will have to modify the CMakeLists.txt file to remove the
the fuzzing targets.

# Structure

## Tokenizer

First we tokenize the input. This stage is well tested and ready.
The tokenizer goes through the whoel input and generates tokens.
It is built as an iterator, therefore it doesn't allocate any memory.

## Parser

I have implemented a recursive descent parser. It is also fully complete
and capable of handling pretty much everything.
It is fault tollerant (it won't stop parsing on the first error)
And it reports nice error messages, showing where the error happened and what was expected.

The parser is also fully tested and fuzzed.

## Semantic analyzer and type checker

From this point on not all features are supported, and no nice error messages will
be generated. If there is an error, the program will halt on an assertion. But all
of them can be replaces with nice error messages. (see the parser for reference how
that would be done)

In this stage we walk recursively through the AST.

On the way donw, we do semantic analysis. This includes:
- making sure that all variables and functions are declared before they are used
- making sure that only the valid syntax is allowed (if expressions vs if statements)

On the way up type checking is done.

Disclaimer: I have never implemented any for of type checking before, and have no
idea how it should be done. I wanted a type system with inference, so this was the idea:

- Everything has a type_set
- type_set represents all possible types that that expression can have
- literals have types by default
- operations enforce types

We basically walk the AST and if we see a function parameter without a type, we assign
it a full type_set (anything is allowed). If we see that this parameter is used in a
binary operation like `+`, with some other value, we do a set_intersection of three sets:
- The left value's set, the right value's set and the operation's set. In this case that
would be `full_set & full_set & {int, float}`. If the intersection is not empty,
we update all the participating values to share the same type set.

This way the types propagate throughout the ast. And we can infer recursion or multiple assignments
easily.

Currently everything has to have one type, therefore a function like this:
```
sum :: (a, b) {
    a + b
}
```

Will not work on `int` and `float` at the same time. The type would be infered from the
first caller, and the second caller would error out.

# Compilation

After semantic analysis there is compilation. I compile the AST to bytecode and then run
that bytecode. This stage is the most incomplete, mainly due to lact of time.
Howevever these features are supported:
- only int types
- only int literals
- only int operations
- if statements only (no if expressions)
- for loops only (no while loops or or expressions)
- function calls
- function definitions
- variable declarations
- variable assignments
- return statements
- binary operations on integers

# VM

After compilation the bytecode is run in a very simple stack based VM.

The VM keeps functions, static data (literal values from code) and the stack
as separate entities. This machine will execute the code and show the results.






