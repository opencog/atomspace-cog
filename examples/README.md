CogServer AtomSpace Shim
------------------------
Allow a CogServer to share it's AtomSpace with other AtomSpaces on
other machines. In ascii-art:

```
 +-------------+
 |             |
 |  CogServer  |  <-----internet------> Remote AtomSpace A
 |             |  <---+
 +-------------+      |
                      +-- internet ---> Remote AtomSpace B

```

Here, AtomSpace A can load/store Atoms (and Values) to the CogServer,
as can AtomSpace B, and so these two can share AtomSpace contents
however desired.

Start the Server
----------------
All of the examples assume you have a cogserver started. This can be
done as (for example):
```
$ guile
scheme@(guile-user)> (use-modules (opencog))
scheme@(guile-user)> (use-modules (opencog cogserver))
scheme@(guile-user)> (start-cogserver)
$1 = "Started CogServer"
scheme@(guile-user)> Listening on port 17001
```

You can also do this in python.

Run the Demos
-------------
The examples go through some simple usage scenarios.

* fetch-store.scm -- Basic fetch and store of single atoms.
* load-dump.scm -- Loading and saving entire AtomSpaces.
