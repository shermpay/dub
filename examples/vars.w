(module vars)

(declare puts (Fn [(Ptr i8)] unit))
(declare printf (Fn [(Ptr i8) i64] unit))

(declare main (Fn [] unit))
(fn main []
  (var x i64)
  (set x 42)
  (printf "x is %d\n" x)
  (var str (Ptr i8))
  (set str "hello")
  (puts str)
  (set str "world")
  (puts str)
  (var y i64 123)
  (printf "y is %d\n" y)
  (return x))
