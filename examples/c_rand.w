(module c_rand)

(declare rand (Fn [] i32))
(declare labs (Fn [i64] i64))

(declare main (Fn [] unit))

(fn main []
  (return (labs -42)))
