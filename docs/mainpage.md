# Overview

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

## Examples

Since pipe-T was designed with usage simplicity in mind, it involves a
significant amount of template trickery and syntax sugar offered by recent C++
standards. We will try now to describe all the stuff in a most natural way for
libraries like that — with examples.

### Simple Examples

Despite these simplest examples are not practically useful, they demonstrates
few basic concepts in a clearest way and may be necessary to got the idea of
more elaborated ones.

#### Single function

The simplest (but unlikely useful) usage example implies constructing a
pipeline with a single handler for arithmetic message type. Type of handler
should be an arbitrary callable accepting mutable reference to message
instance. For the sake of simplicity we impose an ordinary C++ function here,
printing the integer "message":

\snippet 01-simplest/simplest.cpp Simplest handler

Once could further push this handler into the pipeline stack and invoke it
against simple message like this:

\snippet 01-simplest/simplest.cpp Simplest pipeline

Sure, the equivalent functionality may be gained more easily, by just invoking
the `my_processor()` with integer variable argument. But what is important
here, experienced C++ coder may already notice the synctactic sugar that lies
behind of this expressions.

Full code of this example may be found at simplest.cpp file.

#### Pipeline Discrimination

Just invoking the set of callables sequentially is usually not user wants when
it comes to the data processing. Users usually desire to declare a special
behaviour for some specific cases — say, message could be _discriminated_ on
certain stage.

By "discrimination" we imply stopping message instance to be propagated further
in handlers chain. pipe-T's default behaviour for functions returning boolean
values is to stop propagation when they returned `false`. For our next example
illustrating such a processor, let's assume a bit more complicated `Message`
type:

\snippet 02-discriminate/discriminate.cpp A bit more complicated message type

So the simplest discriminating processor that, for instance, cut away messages
with negative `a` values:

\snippet 02-discriminate/discriminate.cpp Simple discriminating handler

To check whether the discrimination logic works, we will introduce a second
type of handling routine that will simply dump the `a` member of incoming
message to `stdout`:

\snippet 02-discriminate/discriminate.cpp Dumping handler

And run all the stuff together with:

\snippet 02-discriminate/discriminate.cpp Discrimination test

The full code of this example may be found at \ref discriminate.cpp file.

### Discussion of simple examples

Besides of synctactic sugar allowing one to put functions with few different
signatures into the ordered container, one could notice that there is a
pre-defined logic hidden under the hood. Particularary, the left-shift
operator (`>>`) getting the message instance invokes the conditional procedure
performing the actual propagation along the pipeline.

The most strightforward implementation involved into the demonstrated examples
may be observed at basic_pipeline.tcc file within the pipet::Pipeline::process()
method. Despite the apparent simplicity of the strightforward pipeline
primitive, we had introduced a special entity to control particular execution
loop — the special arbitering class pipet::PipelineTraits::IArbiter. The
particular role of this class is to decide, whether to propagate message
further, along the pipeline, or whether to continue extraction messages from
special source class. The instances of IArbiter subclasses are not stateless,
and making them correctly is a crucial point to build elaborated pipelines for
practical cases.

At the first glance, for primitive linear pipeline introduction of such a class
looks like an overkill, but we're rarely using the pipeline primitive directly.
Instead, few pipelines are usually organised in a forking structure that here
is called _manifold_.

