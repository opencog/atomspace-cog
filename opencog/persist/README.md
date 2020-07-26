
cog-simple
----------
A very simple implementation of a cogserver-client. Only 500 lines of
code, total.  Just one socket is opened to the cogserver; all server
client communications are serialized on that socket. In particular,
every request waits for a complete response before moving on.
