# Scheme Interpreter

A Scheme language interpreter implemented for fun and C++ practice. It is currently not conforming to the R5RS standard, but it is moving towards it.

It is:
- Simple, compact and easy to read
- Fun project to tackle on weekends

It is not:
- R5RS compliant (yet!)
- Super optimized and fast
- Very sophisticated code-wise

Also for this project I implemented Mark-And-Sweep garbage collector, because I can :)

## Structure
```text
.
├── 3rd_party
│   ├── CMakeLists.txt
│   └── allocations_checker  -- a helper used to test GC (see below)
├── CMakeLists.txt  -- top-level project configuration
├── README.md
├── repl    -- an executable with Read Evaluate Print Loop
├── scheme  -- a static library with the interpreter
└── tests   -- tests for this project using Catch2
```

## Tests
Tests for this project as well as `allocations_checker` is taken from the university course homework that is project originated from.