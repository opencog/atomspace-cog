
AtomSpace CogStorage Client
===========================
The code in this git repo allows an AtomSpace to communicate with
other AtomSpaces by having them all connect to a common CogServer.
The CogServer itself also provides an AtomSpace, which all clients
interact with, in common.  In ascii-art:
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

This provides a very simple, low-brow backend for AtomSpace storage
via the CogServer. At this time, it is ... not optimized for speed,
and its super-simplistic.  It is meant as a proof-of-concept for
a scalable distributed network of AtomSpaces, provided by the
[AtomSpace OpenDHT backend](https://github.com/opencog/atomspace-dht).

Example Usage
-------------
Well, see the examples directory for details. But, in breif:

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
This is Version 0.5. Six of the nine unit tests consistently pass:
 * BasicSaveUTest
 * ValueSaveUTest
 * PersistUTest
 * FetchUTest
 * DeleteUTest
 * MultiUserUTest

Pass, but takes half an hour to run (not surprising, these are big.)
 * LargeFlatUTest
 * LargeZipfUTest

Sometimes hangs for an unknown reason:
 * MultiPersistUTest
