;; NOTES
;; > the testing interface does not support mysql identifiers
;;   (tables, fields, etc) with leading or trailing spaces
;;   + in queries like, ``SELECT a + b FROM t'' mysql returns the field as
;;     'a + b ' (note the trailing space); this behavior is not replicated by
;;     cryptdb hence the tests use a generic string trim operation on
;;     all fields
;; TODO
;; > 'USE' the default database for each test group

(defpackage :cryptdb-testing
  (:use :cl :clsql)
  (:export :main
           :*filter-tests*
           :test-all-units
           :compare-results
           :db-connect
           :issue-query
           :execute-test-query
           :make-connection-state
           :connection-state-cryptdb
           :connection-state-plain
           :destroy-connections
           :read-file))
(in-package :cryptdb-testing)

(proclaim '(optimize (debug 3)))

(defparameter +default-tests-path+       (concatenate 'string
                                           (sb-unix::posix-getenv "EDBDIR")
                                           "/newtesting/tests.sexpr"))
(defparameter +default-ip+               "127.0.0.1")
(defparameter +default-username+         "root")
(defparameter +default-password+         "letmein")
(defparameter +default-database+         "cryptdbtest")
(defparameter +default-control-database+ "cryptdbtest_control")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;     database helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defstruct connection-state
  cryptdb
  plain)

(defmethod destroy-connections ((connections connection-state))
  (clsql:disconnect :database (connection-state-cryptdb connections))
  (setf (connection-state-cryptdb connections) nil)
  (clsql:disconnect :database (connection-state-plain connections))
  (setf (connection-state-plain connections) nil))

(defun db-connect (db port &optional (ip       +default-ip+)
                                     (username +default-username+)
                                     (password +default-password+))
  (clsql:connect
    `(,ip ,db ,username ,password ,port)
    :database-type :mysql
    :if-exists     :new
    :make-default  nil))

(defstruct query-result
  status
  fields
  rows)

(defun issue-query (query database)
  (handler-case
        (multiple-value-bind (rows fields) (clsql:query query :database database)
          (make-query-result
            :status t
            :fields (mapcar #'(lambda (f) (string-trim '(#\Space) f)) fields)
            :rows   rows))
    (clsql:sql-database-error (e)
      (declare (ignore e))
      (make-query-result
        :status nil
        :fields nil
        :rows   nil))))

(defun must-succeed-query (query database)
  (assert (query-result-status (issue-query query database))))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;        misc helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; by _death from #lisp
(defun traverse (function structure path)
  (if (atom structure)
      (funcall function (reverse (cons structure path)))
      (dolist (x (cdr structure))
        (traverse function x (cons (car structure) path)))))

;; by _death from #lisp
(defmacro do-structure (((&rest vars) structure-form) &body forms)
  (let ((list (gensym)))
    `(traverse (lambda (,list)
                 (destructuring-bind ,vars ,list
                   (declare (ignorable ,@vars))
                   ,@forms))
               ,structure-form
               '())))

(defun read-file (path)
  (with-open-file (stream path :direction :input)
    (let ((seq (make-string (file-length stream))))
      (read-sequence seq stream)
      seq)))

(defun str-assoc (key alist)
  (assoc key alist :test #'string-equal))

;;; returns (did-the-lookup-complete? completed-portion-of-lookup unmatched-keys)
(defun many-str-assoc (keys nested-alists)
  (let ((alist nested-alists)
        (keys-len (length keys)))
    (do ((i 0 (1+ i)))
        ((= keys-len i) (values t alist '()))
      (let* ((k (elt keys i))
             (found-alist (str-assoc k (if (zerop i) alist (cdr alist)))))
        (when (null found-alist)
          (return (values nil alist (subseq keys i))))
        (setf alist found-alist)))))

;;; apply fn to each node
(defun map-tree (fn tree)
  (cond ((null tree) '())
        ((atom (car tree))
         (cons (funcall fn (car tree)) (map-tree fn (cdr tree))))
        (t (cons (map-tree fn (car tree)) (map-tree fn (cdr tree))))))

(defun list-depth (list)
  (assert (listp list))
  (cond ((null list) 1)
        (t (apply
             #'max
             (mapcar
               #'(lambda (e)
                   (cond ((atom e) 1)
                         (t (1+ (list-depth e)))))
               list)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;   handle test file input
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun load-tests (&optional (path +default-tests-path+))
  (read-from-string (read-file path)))

;; FIXME: implement
(defun proper-onion-update? (onion-check)
  (assert (listp onion-check))
  t)

(defun build-update-1 (tag database table field onion seclevel)
  `(,tag (,database (,table (,field (,onion ,seclevel))))))

;;; take possibly shorthand onion-checks and produce full length checks
;;; > return nil if `dirty-onion-checks` are invalid
(defun low-level-fixup-onion-checks (dirty-onion-checks default-database)
  (when (atom dirty-onion-checks)
    (return-from low-level-fixup-onion-checks
      (when (eq :check dirty-onion-checks )
        `(,dirty-onion-checks))))
  (let ((default-db-name (if (eq t default-database) +default-database+ default-database))
        (tag (car dirty-onion-checks)))
    (cond ((eq :check tag)
           (when (= 1 (length dirty-onion-checks))
             dirty-onion-checks))
          ((eq :all-max tag)
           (when (every #'atom dirty-onion-checks)
                   ;; (:check <table>)
             (cond ((= 2 (length dirty-onion-checks))
                    `(:all-max ,default-db-name ,(cadr dirty-onion-checks)))
                   ;; (:check <database> <table>)
                   ((= 3 (length dirty-onion-checks))
                    dirty-onion-checks))))
          ((member tag '(:set :update))
           (cond ((every #'atom dirty-onion-checks)
                   ;; (:update <table> <field> <onion> <seclevel>)
                  (cond ((= 4 (length (cdr dirty-onion-checks)))
                         (apply #'build-update-1 tag default-db-name (cdr dirty-onion-checks)))
                        ;; (:update <database> <table> <field> <onion> <seclevel>)
                        ((= 5 (length (cdr dirty-onion-checks)))
                         (apply #'build-update-1 tag (cdr dirty-onion-checks)))))
                   ;; (:update (<database> (<table> ...)) (<database> ...))
                   ((every #'proper-onion-update? (cdr dirty-onion-checks))
                    (cond ((= 4 (list-depth (cdr dirty-onion-checks)))
                           `(,tag (,default-db-name ,@(cdr dirty-onion-checks))))
                          ((= 5 (list-depth (cdr dirty-onion-checks)))
                           dirty-onion-checks))))))))

(defun fixup-onion-checks (dirty-onion-checks default-database)
  (let ((fixed (low-level-fixup-onion-checks dirty-onion-checks default-database)))
    ;; convert symbols to strings except for the initial directive
    (when fixed
      (cons (car fixed) (map-tree #'string (cdr fixed))))))

(defun fixup-execution-target (dirty-execution-target)
  (case dirty-execution-target
    ((nil) :both)
    ((:cryptdb :control :both) dirty-execution-target)))

(defun fixup-testing-strategy (dirty-testing-strategy)
  (case dirty-testing-strategy
    ((nil) :compare)
    ((:compare :must-succeed :must-fail :ignore) dirty-testing-strategy)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;  manage local onion state
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; ((database-name
;;;    (table-name (field-name (onion-name security-level) (onion-name security-level))
;;;                (field-name (...))))
;;;  (database-name
;;;    (table-name (...))
;;;  ...))
(defstruct onion-state
  databases)

(defmethod low-level-lookup-seclevel ((onions onion-state) database table field onion)
  (multiple-value-bind (success lookup unmatched-keys)
        (many-str-assoc `(,database ,table ,field ,onion)
                        (onion-state-databases onions))
    (assert (and success (null unmatched-keys)))
    (cdr lookup)))

(defmethod lookup-seclevel ((onions onion-state) database table field onion)
  (car (low-level-lookup-seclevel onions database table field onion)))

(defun (setf lookup-seclevel) (seclevel onions database table field onion)
  (setf (car (low-level-lookup-seclevel onions database table field onion))
        seclevel))

(defun nest-unmatched-keys (unmatched-keys seclevel)
  (assert (not (null unmatched-keys)))
  (cond ((null (cdr unmatched-keys)) `(,(car unmatched-keys) ,seclevel))
        (t `(,(car unmatched-keys) ,(nest-unmatched-keys (cdr unmatched-keys) seclevel)))))

(defmethod add-onion! ((onions onion-state) database table field onion seclevel)
  ;; don't try to look anything up if we don't have any onion data
  (when (null (onion-state-databases onions))
    (setf (onion-state-databases onions)
          `((,database (,table (,field (,onion ,seclevel))))))
    (return-from add-onion! t))
  (multiple-value-bind (success lookup unmatched-keys)
        (many-str-assoc `(,database ,table ,field ,onion)
                        (onion-state-databases onions))
    ;; a total match should only occur when we max back to back tables
    ;; > ie, consecutive CREATE TABLE queries
    (when success
      (assert (null unmatched-keys))
      (assert (string-equal seclevel (cadr lookup)))
      (return-from add-onion! t))
    (assert (not (null lookup)))
    (setf (cdr lookup)
          (append (cdr lookup)
                  `(,(nest-unmatched-keys unmatched-keys seclevel))))))

(defun max-level? (onion seclevel)
  (cond ((member onion '("oEq" "oOrder" "oPLAIN") :test #'string-equal)
         (string-equal "RND" seclevel))
        ((string-equal "oADD" onion) (string-equal "HOM" seclevel))))

;;; change our local copy of the onion state
(defmethod update-onion-state! ((onions onion-state) onion-check)
  (assert (eq :update (car onion-check)))
  (dolist (checks (cdr onion-check))
    (do-structure ((database table field onion seclevel) checks)
      (setf (lookup-seclevel onions database table field onion) seclevel))))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;    onion check handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defun get-cryptdb-show (connections)
  (let ((results (issue-query "SET @cryptdb='show'"
                              (connection-state-cryptdb connections))))
    (assert (query-result-status results))
    (assert (equal '("_database" "_table" "_field" "_onion" "_level" "id")
                   (query-result-fields results)))
    (query-result-rows results)))

;; should we fail when presented with onion data for a database not in our
;; local cache?
(defparameter *break-errant-database* nil)

(defgeneric handle-check (connections onions type onion-check)
  (:method (connections (onions onion-state) (type (eql :all-max)) onion-check)
    (let ((max-database (cadr onion-check))
          (max-table    (caddr onion-check))
          (results      (get-cryptdb-show connections))
          (output       t))
      (dolist (row results output)
        (destructuring-bind (database table field onion seclevel id) row
          (declare (ignore id))
          (when (and (string-equal max-database database)
                     (string-equal max-table table))
            (when (not (max-level? onion seclevel))
              (setf output nil))
            ;; update our local copy of onion state
            ;; > we cannot use (setf lookup-seclevel) because the
            ;;   local onion state may be nil
            (add-onion! onions database table field onion seclevel))))))
  (:method (connections (onions onion-state) (type (eql :set)) onion-check)
    (dolist (checks (cdr onion-check) t)
      (do-structure ((database table field onion seclevel) checks)
        (unless (add-onion! onions database table field onion seclevel)
          (break)
          (return nil)))))
  (:method (connections (onions onion-state) (type (eql :update)) onion-check)
    (update-onion-state! onions onion-check)
    (handle-check connections onions :check onion-check))
  (:method (connections (onions onion-state) (type (eql :check)) onion-check)
    (let ((results (get-cryptdb-show connections))
          (output  t))
      (dolist (row results output)
        (destructuring-bind (database table field onion seclevel id) row
          (declare (ignore id))
          (cond ((not (str-assoc database (onion-state-databases onions)))
                 (assert (not *break-errant-database*)))
                (t
                  (unless (string-equal seclevel
                             (lookup-seclevel
                               onions database table field onion))
                    ; (break)
                    ;; setting a flag instead of shortcircuiting causes us to
                    ;; continue updating seclevel's after failure
                    (setf output nil))
                  (setf (lookup-seclevel onions database table field onion)
                        seclevel))))))))

;;; take the per query onion checks and use them to update our onion state;
;;; then compare this new onion state to the state reported by cryptdb for
;;; each testing onion
(defmethod handle-onion-checks (connections (onions onion-state) onion-check)
  (if (null onion-check)
      t
      (handle-check connections onions (car onion-check) onion-check)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;         run tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defstruct group-score
  (wins 0)
  (fails 0))

(defgeneric update-score (value)
  (:method ((value (eql t)))
    (declare (special *score*))
    (incf (group-score-wins *score*)))
  (:method ((value (eql nil)))
    (declare (special *score*))
    (break)
    (incf (group-score-fails *score*))))

(defun fast-compare (results-a results-b)
  (equal results-a results-b))

(defun slow-compare (results-aye results-bee)
  (when (not (= (length results-aye) (length results-bee)))
    (return-from slow-compare nil))
  ; (break)
  (every #'(lambda (a)
             (= (count a results-aye :test #'equal)
                (count a results-bee :test #'equal)))
         results-aye))

(defmethod compare-results ((results-a query-result) (results-b query-result))
  (cond ((and (not (query-result-status results-a))
              (not (query-result-status results-b)))
         t)
        ((or (not (query-result-status results-a))
             (not (query-result-status results-b)))
         (break)
         nil)
        ((not (equal (query-result-fields results-a)
                     (query-result-fields results-b)))
         (break)
         nil)
        (t ;; cryptdb returns all results as strings while the normal
           ;; database uses numbers and such
           (let ((rows-a (all-strings (query-result-rows results-a)))
                 (rows-b (all-strings (query-result-rows results-b))))
             (or (fast-compare rows-a rows-b)
                 (slow-compare rows-a rows-b))))))

;; HACK: cryptdb returns integers for SUM(...) of integers while vanilla
;;       mysql returns floating points with no decimals
(defun float-to-string (float)
  (assert (typep float 'float))
  (let ((rational (rationalize float)))
    (if (= 1 (denominator rational))
        (write-to-string (numerator rational))
        (format t "~A" float))))

(defun all-strings (results)
  (mapcar #'(lambda (row)
              (mapcar #'(lambda (e)
                          (etypecase e
                            (string  e)
                            (integer (write-to-string e))
                            (null    "NULL")
                            (float   (float-to-string e))))
                      row))
          results))

(defun valid-group-default? (group-default)
  (or (equal t group-default) (equal nil group-default) (stringp group-default)))

(defun valid-execution-target-and-testing-strategy? (execution-target testing-strategy)
  (case testing-strategy
    ((:must-succeed :must-fail) (member execution-target '(:both :cryptdb)))
    (:compare   (eq :both execution-target))
    (:ignore    t)))

(defmethod execute-test-query ((connections connection-state) query execution-target)
  (let ((cryptdb (connection-state-cryptdb connections))
        (control (connection-state-plain connections)))
    (ecase execution-target
      (:cryptdb (issue-query query cryptdb))
      (:control (values nil (issue-query query control)))
      (:both    (values (issue-query query cryptdb)
                        (issue-query query control))))))

(defun run-test-group (connections test-group)
  (let* ((score (make-group-score))
         (*score* score)
         (onions (make-onion-state))
         (group-name (car test-group))
         (group-default (cadr test-group))
         (test-list (cddr test-group)))
    (declare (special *score*) (ignorable group-name))
    (assert (valid-group-default? group-default))
    (dolist (test-case test-list score)
      (let* ((query (car test-case))
             (onion-checks (fixup-onion-checks (cadr test-case) group-default))
             (execution-target (fixup-execution-target (caddr test-case)))
             (testing-strategy (fixup-testing-strategy (cadddr test-case))))
        ; (format t "~A~%~A~%~A~%~%" query test-case onion-checks)
        (assert (eq (null (cadr test-case)) (null onion-checks)))
        (assert (valid-execution-target-and-testing-strategy?
                  execution-target testing-strategy))
        (multiple-value-bind (cryptdb-results plain-results)
            (execute-test-query connections query execution-target)
          ;; do handle-onion-checks(...) first because we want to update our
          ;; local onions even if the results don't match
          (let ((onion-check-result
                 (handle-onion-checks connections onions onion-checks)))
            ;(format t "~%~%Onion-check-result ~A~%~%" onion-check-result)
            ;(format t "cryptdb results~A~%" cryptdb-results)
            ;(format t "plain results~A~%" plain-results)
            (update-score
              (ecase testing-strategy
                (:must-succeed (query-result-status cryptdb-results))
                (:must-fail    (not (query-result-status cryptdb-results)))
                (:ignore       t)
                (:compare      (and onion-check-result
                                    (compare-results cryptdb-results
                                                     plain-results)))))))))))

(defun run-tests (all-test-groups)
  (let* ((connections
           (make-connection-state :cryptdb (db-connect nil 3307)
                                  :plain   (db-connect nil 3306)))
         (cryptdb (connection-state-cryptdb connections))
         (plain (connection-state-plain connections))
         (results '()))
    ;; remove artefacts
    (issue-query (format nil "DROP DATABASE ~A" +default-database+) cryptdb)
    (issue-query (format nil "DROP DATABASE ~A" +default-control-database+) plain)
    ;; create default database
    (must-succeed-query (format nil "CREATE DATABASE ~A" +default-database+) cryptdb)
    (must-succeed-query (format nil "CREATE DATABASE ~A" +default-control-database+) plain)
    ;; set proper default database
    (must-succeed-query (format nil "USE ~A" +default-database+) cryptdb)
    (must-succeed-query (format nil "USE ~A" +default-control-database+) plain)
    ;; begin testing
    (dolist (test-group all-test-groups results)
      (let ((score (run-test-group connections test-group)))
        (format t "~A:~25T~A failures~%"
                  (car test-group)
                  (group-score-fails score))
        (push score results)))
    (destroy-connections connections)
    (reverse results)))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;   commence cryptdb testing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; give *filter-tests* the names of test groups to be run
;; > no names indicates that all tests should be run
(defparameter *filter-tests* '())

(defun main ()
  (let ((tests (load-tests)))
    (run-tests (remove-if-not #'(lambda (name)
                                  (or (null *filter-tests*)
                                      (member name *filter-tests*
                                              :test #'string-equal)))
                              tests
                              :key #'car))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;        unit tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; FIXME: add tests for unspecified stuff
(defun test-fixup-onion-checks ()
       ;; a fully specifed check should be returned as is
  (and (let ((line '(:update ("database0" ("table0" ("field0" ("onion0" "seclevel0")
                                                              ("onion1" "seclevel1"))
                                                    ("field1" ("onion2" "seclevel2")))))))
         (equal line (fixup-onion-checks line nil)))
       ;; a single check does not require nesting
       (equal '(:update ("database" ("table" ("field" ("onion" "seclevel")))))
              (fixup-onion-checks '(:update "database" "table" "field" "onion" "seclevel") nil))
       (null (fixup-onion-checks '(:update ()) nil))
       (null (fixup-onion-checks :update nil))
       (equal '(:check)
              (fixup-onion-checks '(:check) nil))
       (equal '(:check)
              (fixup-onion-checks :check nil))
       (null (fixup-onion-checks  :all-max nil))
       (equal '(:all-max "db" "table")
              (fixup-onion-checks '(:all-max "db" "table") nil))
       ;; use hardcoded default database
       (equal `(:all-max ,+default-database+ "table")
              (fixup-onion-checks '(:all-max "table") t))
       (equal `(:update (,+default-database+ ("table" ("field" ("onion" "seclevel")))))
              (fixup-onion-checks '(:update "table" "field" "onion" "seclevel") t))
       ;; use default database from test group
       (equal '(:all-max "some-default" "table")
              (fixup-onion-checks '(:all-max "table") "some-default"))
       (equal '(:update ("a-default" ("table" ("field" ("onion" "seclevel")))))
              (fixup-onion-checks '(:update "table" "field" "onion" "seclevel") "a-default"))))

(defun test-fixup-onion-checks-multiple-update-default-bug ()
  (let ((line '(:update ("t" ("f"  ("o"  "l"))
                             ("f2" ("o2" "l2"))))))
    (equal `(:update (,+default-database+ ("t" ("f"  ("o"  "l"))
                                               ("f2" ("o2" "l2")))))
           (fixup-onion-checks line t))))

(defun test-fixup-onion-checks-set ()
  (and (let ((line '(:set ("d" ("t" ("f" ("o" "l")))))))
         (and (equal line (fixup-onion-checks line nil))
              (equal line
                     (fixup-onion-checks '(:set ("t" ("f" ("o" "l")))) "d"))))))

(defun test-fixup-execution-target ()
  (and (eq :both (fixup-execution-target :both))
       (eq :both (fixup-execution-target nil))
       (eq :control (fixup-execution-target :control))
       (eq :cryptdb (fixup-execution-target :cryptdb))
       (eq nil (fixup-execution-target 'whatever))))

(defun test-fixup-testing-strategy ()
  (and (eq :compare (fixup-testing-strategy :compare))
       (eq :must-succeed (fixup-testing-strategy :must-succeed))
       (eq :must-fail (fixup-testing-strategy :must-fail))
       (eq :ignore (fixup-testing-strategy :ignore))
       (eq :compare (fixup-testing-strategy nil))
       (null (fixup-testing-strategy 'whatever))))

(defun test-lookup-seclevel ()
  (let ((onions
          (make-onion-state
            :databases '(("database"
                            ("table0" ("field0" ("onion0" "level")))
                            ("table1" ("field1" ("onion1" "anotherlevel")
                                                ("onion2" "moremore"))
                                      ("field2" ("onion3" "again"))))))))
    (and (string= "level"        (lookup-seclevel onions "database" "table0" "field0" "onion0"))
         (string= "anotherlevel" (lookup-seclevel onions "database" "table1" "field1" "onion1"))
         (string= "moremore"     (lookup-seclevel onions "database" "table1" "field1" "onion2"))
         (string= "again"        (lookup-seclevel onions "database" "table1" "field2" "onion3")))))

(defun test-update-onion-state! ()
  (let ((onions
          (make-onion-state
            :databases '(("database"
                            ("table0" ("field0" ("onion0" "level")))
                            ("table1" ("field1" ("onion1" "anotherlevel")
                                                ("onion2" "moremore"))
                                      ("field2" ("onion3" "again"))))))))
    (update-onion-state! onions '(:update ("database" ("table1" ("field1" ("onion1" "newlevel"))))))
    (string= "newlevel"
             (lookup-seclevel onions "database" "table1" "field1" "onion1"))))

#|
(defun test-multiple-update-onion-state! ()
  (let ((onions
          (make-onion-state
            :databases '(("d" ("t" ("f"  ("o"  "l"))
                                   ("f2" ("o2" "l2"))))))))
    (update-onion-state! onions '(:update ("d" 
  nil)
|#

(defun test-many-str-assoc ()
  (and (multiple-value-bind (success lookup unmatched-keys)
            (many-str-assoc '(a b c) '((a (b (x y)
                                             (c d)))))
         (and success
              (equal '(c d) lookup)
              (null unmatched-keys)))
       (let ((test (copy-list
                     '((a (b (c d)
                             (e f))
                          (c (c d)))))))
         (multiple-value-bind (success lookup unmatched-keys)
            (many-str-assoc '(a c c) test)
           (and success
                (equal '(c d) lookup)
                (null unmatched-keys)
                (multiple-value-bind (success-2 lookup-2 unmatched-keys-2)
                    (many-str-assoc '(a c c) test)
                  (declare (ignore unmatched-keys-2))
                  (assert success-2)
                  (setf (cdr lookup-2) '(g))
                  (equal '((a (b (c d)
                                 (e f))
                              (c (c g))))
                         test)))))
       (multiple-value-bind (success lookup unmatched-keys)
            (many-str-assoc '(x y z) '((x (y)
                                          (z))))
         (and (not success)
              (equal '(y) lookup)
              (equal '(z) unmatched-keys)))
       (multiple-value-bind (success lookup unmatched-keys)
            (many-str-assoc '(x y z)  '((x (y (q)
                                              (p)
                                              (r)))))
         (and (not success)
              (equal '(y (q)
                         (p)
                         (r))
                     lookup)
              (equal '(z) unmatched-keys)))
       (multiple-value-bind (success lookup unmatched-keys)
            (many-str-assoc '(a b c) '((x (y))))
         (and (not success)
              (equal '((x (y))) lookup)
              (equal '(a b c) unmatched-keys)))))

(defun test-add-onion! ()
  (let ((onions (make-onion-state)))
    (add-onion! onions "db0" "t0" "f0" "o0" "l0")
    (and (equal '(("db0" ("t0" ("f0" ("o0" "l0")))))
                (onion-state-databases onions))
         (string= "l0" (lookup-seclevel onions "db0" "t0" "f0" "o0"))
         (progn
           (add-onion! onions "db0" "t0" "f1" "o1" "l1")
           (and (string= "l0" (lookup-seclevel onions "db0" "t0" "f0" "o0"))
                (string= "l1" (lookup-seclevel onions "db0" "t0" "f1" "o1"))))
         (progn
           (add-onion! onions "db0" "t1" "f2" "o1" "l2")
           (and (string= "l0" (lookup-seclevel onions "db0" "t0" "f0" "o0"))
                (string= "l1" (lookup-seclevel onions "db0" "t0" "f1" "o1"))
                (string= "l2" (lookup-seclevel onions "db0" "t1" "f2" "o1")))))))

(defun test-map-tree ()
  (and (equal '((2 3) (4 ((5))) 6)
              (map-tree #'(lambda (n) (1+ n)) '((1 2) (3 ((4))) 5)))
       (equal '()
              (map-tree #'(lambda (n) (declare (ignore n)) (assert nil)) '()))))

(defun test-slow-compare ()
  (and (slow-compare '((a b c)) '((a b c)))
       (not (slow-compare '((a b c)) '((a b c) (a b c))))
       (not (slow-compare '((a b)) '((a b c))))
       (slow-compare '((a b c) (d e f)) '((d e f) (a b c)))
       (slow-compare '((a b c) (d e f) (g h i)) '((d e f) (a b c) (g h i)))
       (not (slow-compare '((a b c) (d e f) (g h i)) '((d e f) (a b c))))))

(defun test-all-strings ()
  (and (equal '(("1" "2" "3" "4.00"))
              (all-strings '((1 2.0 3.000 "4.00"))))))

(defun test-list-depth ()
  (and (= 1 (list-depth '()))
       (= 1 (list-depth '(2 3 4)))
       (= 2 (list-depth '(1 (2) (4) 7)))
       (= 3 (list-depth '(1 (2) 3 4 (5 (6) 7))))))

(defun test-all-units ()
  (and (test-fixup-onion-checks)
       (test-fixup-onion-checks-multiple-update-default-bug)
       (test-fixup-onion-checks-set)
       (test-fixup-execution-target)
       (test-fixup-testing-strategy)
       (test-lookup-seclevel)
       (test-update-onion-state!)
       (test-many-str-assoc)
       (test-add-onion!)
       (test-map-tree)
       (test-slow-compare)
       (test-all-strings)
       (test-list-depth)))

