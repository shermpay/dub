(module structs)

(type Point (struct [(x i64) (y i64)]))

(declare printf (Fn [(Ptr i8) i64] unit))

(declare main (Fn [] unit))
(fn main []
  (var p Point)
  (set (. p x) 42)
  (printf "test: %d\n" 42)
  (return 0))
