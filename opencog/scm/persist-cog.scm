;
; OpenCog CogServer Persistence module
;

(define-module (opencog persist-cog))

(use-modules (opencog))
(use-modules (opencog cog-config))
(load-extension
	(string-append opencog-ext-path-persist-cog "libpersist-cog")
	"opencog_persist_cog_init")

(export cog-storage-clear-stats cog-storage-close cog-storage-open
cog-storage-stats cog-storage-load-atomspace)

; --------------------------------------------------------------

(set-procedure-property! cog-storage-clear-stats 'documentation
"
 cog-storage-clear-stats - reset the performance statistics counters.
    This will zero out the various counters used to track the
    performance of the CogServer backend.  Statistics will continue to
    be accumulated.
")

(set-procedure-property! cog-storage-close 'documentation
"
 cog-storage-close - close the currently open CogServer backend.
    Close open connections to the currently-open CogServer, after flushing
    any pending writes in the write queues. After the close, atoms can
    no longer be stored to or fetched from the CogServer.
")

(set-procedure-property! cog-storage-open 'documentation
"
 cog-storage-open URL - Open a connection to a CogServer.

  The URL must be one of these formats:
     cog://HOSTNAME/
     cog://HOSTNAME:PORT/

  If no hostname is specified, its assumed to be 'localhost'. If no port
  is specified, its assumed to be 17001.

  Examples of use with valid URL's:
     (cog-storage-open \"cog://localhost/\")
     (cog-storage-open \"cog://localhost:17001/\")
")

(set-procedure-property! cog-storage-stats 'documentation
"
 cog-storage-stats - report performance statistics.
    This will cause some AtomSpace performance statistics to be printed
    to stdout. These statistics can be quite arcane and are useful
    primarily to the developers of the server.
")
