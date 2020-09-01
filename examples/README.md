CogServer AtomSpace Shim
------------------------
Allow a CogServer to share it's AtomSpace with other AtomSpaces on
other machines. In ASCII-art:

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

You can connect to more than one server at the same time, and thus
create a distributed network:
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

Start the Server
----------------
All of the examples assume you have a cogserver started. This can be
done as (for example):
```
$ guile
scheme@(guile-user)> (use-modules (opencog) (opencog exec))
scheme@(guile-user)> (use-modules (opencog cogserver))
scheme@(guile-user)> (start-cogserver)
$1 = "Started CogServer"
scheme@(guile-user)> Listening on port 17001
```

You can also do this in python (i.e. start and run the cogserver in
python).  If you don't have the cogserver, well:
[go back and get it](https://github.com/opencog/cogserver).

Run the Demos
-------------
The first two examples go through some simple usage scenarios. The third
example shows how to make fine-tuned, narrow and precise fetches of
data from the remote server. The fourth example shows how to connect
to multiple servers at the same time, trading data between each of them.

* [fetch-store.scm](fetch-store.scm) -- Basic fetch and store of single atoms.
* [load-dump.scm](load-dump.scm) -- Loading and saving entire AtomSpaces.
* [remote-query.scm](remote-query.scm) -- Precisely-specified fetches.
* [decentralized.scm](decentralized.scm) -- Using several servers at the same time.

Using the Simple Client
-----------------------
The examples should work equally well with the simple client.
Use
```
(use-modules (opencog persist-cog-simple))
```
to load the correct API, and then use
```
(cog-simple-open "cog://localhost")
```
to use the simple API.
