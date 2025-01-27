
AtomSpace CogStorage Client
===========================
<!--
[![CircleCI](https://circleci.com/gh/opencog/atomspace-cog.svg?style=svg)](https://circleci.com/gh/opencog/atomspace-cog)
-->

The code in this git repo allows an AtomSpace to communicate with
other AtomSpaces by having them all connect to a common CogServer.
The CogServer itself also provides an AtomSpace, which all clients
interact with, in common.  In ASCII-art:
```
 +-------------+
 |  CogServer  |
 |    with     |  <-----internet------> Remote AtomSpace A
 |  AtomSpace  |  <---+
 +-------------+      |
                      +-- internet ---> Remote AtomSpace B

```

Here, AtomSpace A can load/store Atoms (and Values) to the CogServer,
as can AtomSpace B, and so these two can share AtomSpace contents
however desired.

This provides a simple, straight-forward backend for networking
together multiple AtomSpaces so that they can share data. This
backend, together with the file-based (RocksDB-based) backend
at [atomspace-rocks](https://github.com/opencog/atomspace-rocks)
is meant to provide a building-block out of which more complex
distributed and/or decentralized AtomSpaces can be built.

This really is decentralized: you can talk to multiple servers at once.
There is no particular limit, other than that of bandwidth,
response-time, etc.  In ASCII-art:

```
 +-----------+
 |           |  <---internet--> My AtomSpace
 |  Server A |                      ^  ^
 |           |        +-------------+  |
 +-----------+        v                v
                 +----------+   +-----------+
                 |          |   |           |
                 | Server B |   |  Server C |
                 |          |   |           |
                 +----------+   +-----------+
```

Here's yet another diagram, reflecting the actual usage in the
[LinkGrammar AtomSpace dictionary](https://github.com/opencog/link-grammar/tree/master/link-grammar/dict-atomese).
Stacked boxes represent shared-libraries, with shared-library calls going
downwards. Note that AtomSpaces start out empty, so the data has to come
"from somewhere". In this case, the data comes from another AtomSpace, running
remotely (in the demo, its in a Docker container).  That AtomSpace in turn
loads its data from a
[RocksStorageNode](https://github.com/opencog/opencog-rocks), which uses
[RocksDB](https://rocksdb.org) to work with the local disk drive.
The network connection is provided by a
[CogServer](https://github.com/opencog/cogserver) to
[CogStorageNode](https://github.com/opencog/opencog-cog) pairing.
```
                                            +----------------+
                                            |  Link Grammar  |
                                            |    parser      |
                                            +----------------+
                                            |   AtomSpace    |
    +-------------+                         +----------------+
    |             |                         |                |
    |  Cogserver  | <<==== Internet ====>>  | CogStorageNode |
    |             |                         |                |
    +-------------+                         +----------------+
    |  AtomSpace  |
    +-------------+
    |    Rocks    |
    | StorageNode |
    +-------------+
    |   RocksDB   |
    +-------------+
    | disk drive  |
    +-------------+
```


Example Usage
-------------
Well, see the examples directory for details. But, in brief:

* Start the CogServer at "example.com":
```
$ guile
scheme@(guile-user)> (use-modules (opencog))
scheme@(guile-user)> (use-modules (opencog cogserver))
scheme@(guile-user)> (start-cogserver)
$1 = "Started CogServer"
scheme@(guile-user)> Listening on port 17001
```
Then create some atoms (if desired)

* On the client machine:
```
$ guile
scheme@(guile-user)> (use-modules (opencog))
scheme@(guile-user)> (use-modules (opencog persist))
scheme@(guile-user)> (use-modules (opencog persist-cog))
scheme@(guile-user)> (cogserver-open "cog://example.com/")
scheme@(guile-user)> (load-atomspace)
```

That's it! You've copied the entire AtomSpace from the server to
the client!  Of course, copying everything is generally a bad idea
(well, for example, its slow, when the atomspace is large). More
granular load and store is possible; see the
[examples directory](examples) for details.

Status
------
This is **Version 1.1.0**. All 26 (13+13) unit tests consistently
pass. (*) (There are occasional crashes during shutdown, in the
shared-library dtor, after the unit tests have passed. See
[cogutil issue #247](https://github.com/opencog/cogutil/issues/247)
for details. Tracked locally as
[atomspace-cog issue #2](https://github.com/opencog/atomspace-cog/issues/2).)

Performance looks good. Two of the unit tests take about 20 seconds
each to run; two more take a few minutes.  This is intentional,
they are pounding the server with large datasets.

This is a "stable" version. There are no known bugs at this time.
It is being used in "production" environments, successfully transferring
gigabytes of data around.

There is one missing feature, but no one uses it (yet): support for
multiple atomspaces (aka frames) is missing. Work on adding this was
started but is low priority.  One unit test works. See the `cog-simple`
directory.

Design
------
There are actually two implementations in this repo. One that is
"simple", and one that is multi-threaded and concurrent (and so
should have a higher throughput). Both "do the same thing",
functionally, but differ in network usage, concurrency, etc.

### The Simple Backend
This can be found in the [opencog/persist/cog-simple](opencog/persist/cog-simple)
directory.  The grand-total size of this implementation is less than 500
lines of code. Seriously! This is really a very simple system!  Take a
look at [CogSimpleStorage.h](opencog/persist/cog-simple/CogSimpleStorage.h)
first, and then take a look at
[CogSimpleIO.cc](opencog/persist/cog-simple/CogSimpleIO.cc)
which does all of the data transfer to/from the cogserver. Finally,
[CogSimpleStorage.cc](opencog/persist/cog-simple/CogSimpleStorage.cc)
provides init and socket I/O.

This backend can be accessed via:
```
scheme> (use-modules (opencog persist-cog-simple))
scheme> (define cssn (CogSimpleStorageNode "cog://example.com/"))
scheme> (cog-open cssn)
```

### The Production Backend
This backend opens four sockets to the cogserver, and handles requests
asynchronously. In other words, requests might be handled out-of-order.
If there is some critical code segment that can't tolerate this, use the
`(barrier)` call. It will flush the network buffers, and force a
serialization barrier at the remote end. The `(barrier)` is a 'fence'
and not a synchronization chekpoint: it ensures that all reads/writes
before the barrier are completed before any that come after are started.

Usage is much like before:
```
scheme> (use-modules (opencog persist-cog))
scheme> (define csn (CogStorageNode "cog://example.com/"))
scheme> (cog-open csn)
```

URL's
-----
Supported URL's include:
* `cog://example.com/` -- standard internet hostname
* `cog://1.2.3.4/` -- standard dotted IPv4 address
* `cog://example.com:17001` -- specify the port of the cogserver.

See
[proxying](https://github.com/opencog/atomspace/tree/master/opencog/persist/proxy)
for details about how the cogserver can pass on I/O requests to other
storage nodes.
