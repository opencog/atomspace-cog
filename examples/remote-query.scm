;
; remote-query.scm
;
; Demo of using queries to obtain sharply-defined sets of Atoms
; from the remote server. These queries generalize the idea of
; fetching the incoming set, or fetching the subset of Atoms of
; some given type, by allowing a highly-detailed search pattern
; to be sent to the remote server, to get exactly those atoms that
; you want.
;
; There are three generic types of queries: MeetLinks, JoinLinks
; and QueryLinks.
; * MeetLinks resemble the idea of a "meet" from set theory (or
;   lattice theory): they effectively say "get me all Atoms that
;   satisfy properties X and Y and Z". The wiki, and atomspace
;   tutorials provide much, much more information.
; * JoinLinks resemble the idea of a "join" from set theory (or
;   lattice theory): they effectively say "get me everything that
;   contains Atom X". Again, the wiki and other examples explain
;   more.
; * QueryLinks are rewrite rules: they perform a Meet, and then
;   take those results to create some new structure.
;
; Each of these query types can be used to fetch Atoms from the
; remote server.
;
; -------------------------------
; Basic initialization and set-up
;
(use-modules (opencog) (opencog persist))
(use-modules (opencog persist-cog))
(cogserver-open "cog://localhost")

; Or, instead, for the simple client, say
; (use-modules (opencog persist-cog-simple))
; (cog-simple-open "cog://localhost")

; --------------
; Populate the Atmspace.
;
(cog-get-all-roots)

; That's all! Thanks for paying attention!
; ----------------------------------------
