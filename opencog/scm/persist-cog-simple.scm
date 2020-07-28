;
; OpenCog CogServer Persistence module
;

(define-module (opencog persist-cog-simple))

(use-modules (opencog))
(use-modules (opencog cog-config))
(load-extension
	(string-append opencog-ext-path-persist-cog-simple "libpersist-cog-simple")
	"opencog_persist_cog_simple_init")

(export cog-simple-clear-stats cog-simple-close cog-simple-open
cog-simple-stats cog-simple-load-atomspace)

; --------------------------------------------------------------

(set-procedure-property! cog-simple-clear-stats 'documentation
"
 cog-simple-clear-stats - reset the performance statistics counters.
    This will zero out the various counters used to track the
    performance of the CogServer backend.  Statistics will continue to
    be accumulated.
")

(set-procedure-property! cog-simple-close 'documentation
"
 cog-simple-close - close the currently open CogServer backend.
    Close open connections to the currently-open backend, after flushing
    any pending writes in the write queues. After the close, atoms can
    no longer be stored to or fetched from the database.
")

(set-procedure-property! cog-simple-open 'documentation
"
 cog-simple-open URL - Open a connection to a CogServer.

  The URL must be one of these formats:
     cog://HOSTNAME/
     cog://HOSTNAME:PORT/

  If no hostname is specified, its assumed to be 'localhost'. If no port
  is specified, its assumed to be 17001.

  Examples of use with valid URL's:
     (cog-simple-open \"cog://localhost/\")
     (cog-simple-open \"cog://localhost:17001/\")
")

(set-procedure-property! cog-simple-stats 'documentation
"
 cog-simple-stats - report performance statistics.
    This will cause some database performance statistics to be printed
    to the stdout of the server. These statistics can be quite arcane
    and are useful primarily to the developers of the server.
")
