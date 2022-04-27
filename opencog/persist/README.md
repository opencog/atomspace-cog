
cog-simple
----------
A very simple implementation of a cogserver-client. Only 500 lines of
code, total.  Just one socket is opened to the cogserver; all server
client communications are serialized on that socket. In particular,
every request waits for a complete response before moving on.

cog-storage
-----------
A multi-threaded, parallel implementation of a cogserver-client. Far
more complex, but should allow greater throughput for complex production
uses. Read `cog-simple` to understand the general idea, but use this in
practice.

cog-common
----------
Snippets of code shared by both the above.
