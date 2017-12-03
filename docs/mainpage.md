# pipe-T — C++ Pipeline Template Classes

The main purpose of the library is to provide simple approach for organizing
dataflow libraries inside larger APIs using generic programming framework
(namely, C++ templates).

Despite one could find a lot of similar software, the pipe-T provides a few of
unique features:

* It is a pure-template library that leaves the particular types and
synchronization primitives to user-side code
* It provides pre-define macros for SWIG offering to user easy tools for making
wrappers onto various languages
* Exploiting a simplistic concept, the architectural logic of pipe-T insists on
transparent intuitive primitives used as a building blocks to become more
user-friendly

It is not a comprehensive tool for parallel computations. pipe-T is just a
template front-end to utilize complex things like
[CAF](https://www.actor-framework.org/) or
[RaftLib](https://github.com/RaftLib/RaftLib), etc. This project is a
middleware attempt connecting these elaborated solutions and higher-leveled
environments like Python/R/CINT.

