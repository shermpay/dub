;; Simple Module
(module add)

(declare printf (Fn [(Ptr i8) i64] unit))

 ;; Testing comments
 ;;  with multiple lines  
(declare main (Fn [] unit))
(fn main []
  (var x i64 (add-i64 40 2))
  ;; (var x i64 42)
  (printf "(add-i64 40 2) => %d\n" x)
  ;; Return is required
  (return 0))
