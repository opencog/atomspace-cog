;
; OpenCog CogServer Persistence module
;

(define-module (opencog persist-cog-simple))

(use-modules (opencog))
(use-modules (opencog cog-config))
(load-extension
	(string-append opencog-ext-path-persist-cog-simple "libpersist-cog-simple")
	"opencog_persist_cog_simple_init")

(export cog-simple-close cog-simple-open)

; --------------------------------------------------------------

(set-procedure-property! cog-simple-close 'documentation
"
 cog-simple-close - close the currently open CogServer backend.
    Close open connections to the currently-open CogServer, after flushing
    any pending writes in the write queues. After the close, atoms can
    no longer be stored to or fetched from the CogServer.
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
