;
; load-dump.scm
;
; Demo showing how to load, or dump, large segments of the AtomSpace,
; including the ENTIRE AtomSpace. Caution: for large AtomSpaces, loading
; everything can be slow, and is generally not needed. Thus, one can
; load portions of the AtomSpace:
;
; load-referers ATOM -- to load only those graphs containing ATOM
; load-atoms-of-type TYPE -- to load only atoms of type TYPE
; load-atomspace -- load everything.
; fetch-query QUERY -- load all Atoms satisfying QUERY.
;
; store-referers ATOM -- store all graphs that contain ATOM
; store-atomspace -- store everything.
;
; -------------------------------
; Basic initialization and set-up
;
(use-modules (opencog) (opencog persist))
(use-modules (opencog persist-cog))

(cogserver-open "cog://localhost")

; ----------------------------------
; Load Atoms from the remote server.
;
; Start by assuming the remote server has some content. If not, then
; create some. If unsure how, re-read the `fetch-store.scm` demo.
; This demo works best if you run it after the `fetch-store.scm` demo,
; and leave the cogserver running. That way, you'll be familiar with
; what is being held there, and what the examples below will be fetching.
;
; Let's get only those atoms that make use of `(Concept "a")`
(load-referers (Concept "a"))

; Print the atomspace contents, and look what we got.
(cog-get-all-roots)

; Now get all PredicateNodes. With luck, there should be at least one,
; the key used to store TruthValues.
(load-atoms-of-type 'Predicate)
(cog-get-all-roots)

; What the heck -- get everything.
(load-atomspace)
(cog-get-all-roots)

; Let's take a look at some low-level I/O statistics.
(cogserver-stats)

; ---------------------------------
; Store Atoms to the remote server.
;
; Let's create some Atoms, and then store everything.
(Concept "foo" (stv 0.1 0.2))
(Concept "bar" (stv 0.3 0.4))
(Set (List (Set (List (Concept "bazzzz" (stv 0.5 0.6))))))
(store-atomspace)

; Let's take a look at those low-level I/O stats again.
(cogserver-stats)

; That's all folks!  Thanks for paying attention!
; -----------------------------------------------
