(in-package :sym)

(defun make-symvec (&key (dimensionality 0 dim-supp-p))
  (let ((s (make-instance 'symvec)))
    (when dim-supp-p
      (setf (vec s) (make-array dimensionality))
      (zero s))
    s))

(defmethod compose ((s symvec) &rest elements)
  (setf (vec s) (coerce elements 'vector))
  s)

(defmethod decompose ((s symvec))
  (coerce (vec s) 'list))

(defmethod dimensionality ((s symvec))
  (length (vec s)))

(defmethod distance ((sym1 symvec) (sym2 symvec))
  (let ((dim1 (dimensionality sym1))
        (dim2 (dimensionality sym2))
        (sum 0))
    (assert (= dim1 dim2))
    (dotimes (i dim1)
      (let ((tmp (- (aref (vec sym2) i)
                    (aref (vec sym1) i))))
        (incf sum (* tmp tmp))))
    (sqrt sum)))

(defmethod distance-estimate ((sym1 symvec) (sym2 symvec))
  ;; Don't compute the sqrt, just return the sum.
  (let ((dim1 (dimensionality sym1))
        (dim2 (dimensionality sym2))
        (sum 0))
    (assert (= dim1 dim2))
    (dotimes (i dim1)
      (let ((tmp (- (aref (vec sym2) i)
                    (aref (vec sym1) i))))
        (incf sum (* tmp tmp))))
    sum))

(defmethod zero ((s symvec))
  (dotimes (i (dimensionality s))
    (setf (aref (vec s) i) 0.0))
  s)

(defmethod one ((s symvec))
  (dotimes (i (dimensionality s))
    (setf (aref (vec s) i) 1.0))
  s)

(defmethod randomize ((s symvec))
  (dotimes (i (dimensionality s))
    (setf (aref (vec s) i) (random 1.0)))
  s)

(defmethod add ((out symvec) &rest syms)
  out)

(defmethod multiply ((out symvec) &rest syms)
  out)

(defmethod stdout (s (sym symvec))
  (format s "[symvec]: ~A~%" (vec sym)))

(defmethod interpolate ((sym-start symvec) (sym-end symvec) (u number))
  sym-start)

(defmethod copy ((s symvec))
  s)

(defmethod move ((sym-dest symvec) (sym-src symvec))
  sym-dest)

(defmethod abstract ((s symvec) &rest syms)
  (values s :xxx-schema))

(defmethod unabstract ((s symvec) schema)
  (list))


