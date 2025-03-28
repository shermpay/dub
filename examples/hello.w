(module hello)

(declare puts (Fn [(Ptr i8)] unit))

(declare main (Fn [] unit))
(fn main []
  (puts "hello world!")
  (return 0))