;
; OpenCog CogServer Persistence module
;

(define-module (opencog persist-cog))

(use-modules (opencog))
(use-modules (opencog cog-config))
(load-extension
	(string-append opencog-ext-path-persist-cog "libpersist-cog")
	"opencog_persist_cog_init")

(export cogserver-clear-stats cogserver-close cogserver-open
cogserver-stats cogserver-load-atomspace)

; --------------------------------------------------------------

(set-procedure-property! cogserver-clear-stats 'documentation
"
 cogserver-clear-stats - reset the performance statistics counters.
    This will zero out the various counters used to track the
    performance of the CogServer backend.  Statistics will continue to
    be accumulated.
")

(set-procedure-property! cogserver-close 'documentation
"
 cogserver-close - close the currently open CogServer backend.
    Close open connections to the currently-open CogServer, after flushing
    any pending writes in the write queues. After the close, atoms can
    no longer be stored to or fetched from the CogServer.
")

(set-procedure-property! cogserver-open 'documentation
"
 cogserver-open URL - Open a connection to a CogServer.

  The URL must be one of these formats:
     cog://HOSTNAME/
     cog://HOSTNAME:PORT/

  If no hostname is specified, its assumed to be 'localhost'. If no port
  is specified, its assumed to be 17001.

  Examples of use with valid URL's:
     (cogserver-open \"cog://localhost/\")
     (cogserver-open \"cog://localhost:17001/\")
")

(set-procedure-property! cogserver-stats 'documentation
"
 cogserver-stats - report performance statistics.
    This will cause some AtomSpace performance statistics to be printed
    to stdout. These statistics can be quite arcane and are useful
    primarily to the developers of the server.
")
