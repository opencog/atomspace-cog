;
; decentralized.scm
;
; Demo of decentralized operation, where the local AtomSpace is
; connected to two or more cogservers, trading data with each of them.
; This demo requires you to configure at least two cogservers on
; different machines, or to run two cogservers on the same machine,
; with different port numbers. See the cogserver docs for how to do
; this.
;
; ----------------------------------------------
; Getting started, making the initial connection.
;
(use-modules (opencog) (opencog persist))
(use-modules (opencog persist-cog))
(use-modules (opencog persist-cog-simple))

; Let's talk to the two machines in two different ways.
; The CogSimpleStorageNode uses the simple, single-threaded
; serialized communications channel; the CogStorageNode uses
; the parallelized, asynchronous channel.
(define csna (CogStorageNode "cog://192.168.1.1"))
(define csnb (CogSimpleStorageNode "cog://192.168.1.2"))

; Open the channel to each; by default, storage nodes are created
; with closed channels. This allows storage nodes to be created,
; when the remote end is not actually available, yet.
(cog-open csna)
(cog-open csnb)

; --------------
; Moving Atoms
;
; On server A, create an atom `(Concept "foo" (stv 0.9 0.2))`,
; then fetch it to here:
(fetch-atom (Concept "foo") csna)

; Push it out to server B:
(store-atom (Concept "foo") csnb)

; Verify, on server B, that the (Concept "foo") showed up, and has the
; correct TruthValue on it.

; --------------
; That's it! You've created a distributed AtomSpace! All of the other
; storage commands work the same way.  For example, you can copy the
; entire atomspace from A to here, and from here to B:
(load-atomspace csna)
(store-atomspace csnb)

; --------------
; When database people say "eventual consistency", they mean there is
; no "universal truth". We can demo this explicitly.

; On server A, create an atom `(Concept "bar" (stv 0.1111 0.87777))`
; On server B, create an atom `(Concept "bar" (stv 0.67 0.99))`

; Ask each one in turn:
(fetch-atom (Concept "bar") csna)
(fetch-atom (Concept "bar") csnb)

(fetch-atom (Concept "bar") csna)
(fetch-atom (Concept "bar") csnb)

(fetch-atom (Concept "bar") csna)
(fetch-atom (Concept "bar") csnb)

; Note how the TV toggled with each fetch. Lets make things
; "eventually consistent":
(Concept "bar" (stv 0.3 0.4))
(store-atom (Concept "bar") csna)
(store-atom (Concept "bar") csnb)

; --------------
; We can be nice and shutdown cleanly:
(cog-close csna)
(cog-close csnb)

; --- The end. That's all folks! ----
