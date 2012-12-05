(in-package :sym)

;; The generic symbol used
(defclass sym ()
  ())

;; A symbol which is a point in high dimensional space.
(defclass symvec (sym)
  ((%vec :initarg :vec
         :initform nil
         :accessor vec)))


