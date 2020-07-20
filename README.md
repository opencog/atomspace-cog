
AtomSpace CogStorage Client
===========================
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

This provides a simple, unsophisticated backend for AtomSpace storage
via the CogServer. At this time, it is ... not optimized for speed,
and is super-simplistic.  It is meant as a proof-of-concept for
a scalable distributed network of AtomSpaces; one possible design being
explored is the
[AtomSpace OpenDHT backend](https://github.com/opencog/atomspace-dht).

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
This is Version 0.5. All nine unit tests consistently pass. Performance
looks good. Five of the unit tests take a minute or longer to run; this
is intentional, they are pounding the server with multiple threads,
looking for race conditions.

XXX Well, not quite true, there's a bug in the cogserver that prevents
MultiPersistUTest from running at full speed. Looking into it...

Design
------
The grand-total size of the implementation is less than 500 lines of
code. Seriously! This is really a very simple system!  Take a look at
[CogStorage.h](opencog/persist/cog-storage/CogStorage.h) first, and
then take a look at [CogIO.cc](opencog/persist/cog-storage/CogIO.cc)
which does all of the data transfer to/from the cogserver. Finally,
[CogStorage.cc](opencog/persist/cog-storage/CogStorage.cc) provides
init and socket I/O.