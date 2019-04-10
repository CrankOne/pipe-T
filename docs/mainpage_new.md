
# Glossary

**Message** is a base, atomic portion of information. Typical example of
*message* is a single frame in video stream, single event in physical process,
tomography imaging slice, etc.

**Processor** is a base representation of single processing function within the
pipeline. Each processor performs simple operation that may or may not affect
the *message* data. Depending on the side effects caused by processor we
distinguish the *processor* to be a *stateful* (functors) or *stateless* (that
is supposed to be pure functions). Depending on whether the *processor*
transforms the *message* data, we distinguish *observer*s and *mutator*s. Upon
completion the *processor* must return a special object, the *result code*,
which drives the *message propagation* procedure.

**Observer** does not make changes on *message* data. Typical cases of use are
various histograms, plotters, sum accumulators, etc.

**Mutator** introduces changes on *message* data: normalization, injection of
additional data, translation, decoding, etc.

**Message propagation** procedure is an algorithm of sequential applying
*processor*s to the single *message*. This procedure might be driven by *result
code* objects returned by *processor*s.

**Pipeline** is a chain of *processors* that have to be sequentially applied to
*message* (*message propagation*). Each processor in a pipeline may block the
propagation of *message* by returning specific *result code*.

**Spanning** is a technique of conjugating pipelines of two different types,
where one message type (**external**) holds another (**internal**).

**Extraction** procedure is an algorithm of sequential applying the *message
propagation* over an array of *messages*.

# Concepts

## Message Propagation and Spanning

In case of propagation of single message, the envisaged procedure considers to
major cases that may affect the strightforward logic:

1. Message propagation may be aborted by any processor
2. Message might be modified within one or more of the mutators

In case of abort result code returned by any processor in chain, no further
processors will be invoked with the current message object, the result will
be forwarded out from encompassed routine.

Mutators have to indicate whether or not the message data was modified by
returning corresponding code. Despite the returning code type itself is defined
via template traits specification it is assumed that:

1. Returned code object may be constructed from integer value `0`.
2. `0` has to be interpreted as "modification was done" and "no abort" return
code.

To indicate that no modification was performed, the
`Traits<Message>::Routing::mark_intact()` function is applied to `0` return
code object.

This "was intact?" feature is used during the message propagation procedure
within sub-pipelines that perform so-called (un-)packing: *spans*. Spans are
special processor kind that encapsulates pipelines of message type different
then that is used outside of span -- we thus call them *internal type* and
*external type* -- and performs internal conversion.

Typical use case of the spanning technique is when the message of external type
carries serialized, compressed or by any mean, encoded data that have to be
translated into sub-messages of internal type. Upon modification the user code
may want to encode the sub-message(s) back, and *spans* are the mean to support
this case. The practical rationale behind the spanning technique is that many
popular data serialization formats (like google protocol buffers, ZeroMQ, etc.)
may be naturally immersed within such a paradigm.

## Message Arrays Iteration -- Extraction

Another important use case of the pipelines is iterationg over a bulk sources
of data, when large amount of messages has to be read sequentially and treated
using pipeline in an independent manner with ability for any processor to
immediately abort message iteration upon certain condition is met (e.g. message
look-up).

For this case, the `ExtractionTraits` type have to be defined describing of how
such an iteration procedure must be performed.

## Parallelism Strategies

Two different techniques of parallelism may be considered in case of
non-trivial pipelines:

1. Processing using few pipelines, each in separate execution context (thread
or process)
2. Processing one pipeline using parallel execution contexts (thread or
process)

First strategy requires advanced techniques of merging and/or synchronization
for stateful processors and higly depends on user code, while second might not
benefit at all if the pipeline itself is not uniform in sense of computational
costs. Second strategy, however might be generalized in a well-defined manner:
by providing each generic processor with condition variable indicating its
availibility, one may steer the message propagation in a way that maximizes
utilization of available CPU time.

# Alternatives and Relative Software

* Apache [RaftLib]()
* Intel [Threading Building Blocks](http://threadingbuildingblocks.org/)
* Boost's [Dataflow](https://web.archive.org/web/20160605053459/http://dancinghacker.com/code/dataflow/dataflow/introduction/dataflow.html) framework

# References



