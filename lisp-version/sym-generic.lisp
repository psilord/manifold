(in-package :sym)


(defgeneric compose (sym &rest elements)
  (:documentation
   "Return a symbol composed of the elements."))

(defgeneric decompose (sym)
  (:documentation
   "Return a list of elements corresponding to the dimensional components
of the symbol."))

(defgeneric distance (sym1 sym2)
  (:documentation
   "Return the distance between two symbols"))

(defgeneric distance-estimate (sym1 sym2)
  (:documentation
   "Return a fast estimate of the ditance etween sym1 and sym2"))

(defgeneric zero (sym)
  (:documentation
   "In the algebraic ring of symbols, make this the 'zero' symbol"))

(defgeneric one (sym)
  (:documentation
   "In the algebraic ring of symbols, make this the 'one' symbol"))

(defgeneric randomize (sym)
  (:documentation
   "Fill the symbol with random, but normalized, data"))

(defgeneric dimensionality (sym)
  (:documentation
   "Return the dimensionality of the symbol"))

(defgeneric add (out &rest args)
  (:documentation
   "Add the argument syms and put results into out"))

(defgeneric multiply (out &rest args)
  (:documentation
   "Multiply the argument syms and put results into out"))

(defgeneric stdout (str sym)
  (:documentation
   "Emit the symbol to the stream"))

(defgeneric interpolate (sym1 sym2 u)
  (:documentation
   "Interpolate from sym1 to sym2 by amount u from 0 to 1"))

(defgeneric copy (sym)
  (:documentation
   "Return a copy of the symbol"))

(defgeneric move (sym-dest sym-src)
  (:documentation
   "Move the src symbol onto the dest symbol"))

(defgeneric abstract (sym &rest syms)
  (:documentation
   "Assemble the symbols into the passed in symbol via an abstraction process.
Return the values of the abstracted symbol and the schema. It should be
that the dimensionality of the new symbol is the sum of the symbols."))

(defgeneric unabstract (sym schema)
  (:documentation
   ("Disassemble a symbol into a collection of other symbols according to
the original assembly schema. The dimensionality of each unabstracted symbol
should sum to the dimensionality of the input symbol.")))
