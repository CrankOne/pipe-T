
# Concepts

## Glossary

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

## Message Propagation and Spanning

In case of propagation of single message, the envisaged procedure considers to
major cases that may affect the strightforward logic:

1. Message propagation may be aborted by any processor
2. Message might be modified within one or more of the mutators

In case of abort result code returned by any processor in chain, no further
processors will be invoked with the current message object, the result will
be forwarded up (returned from encompassed routine).


