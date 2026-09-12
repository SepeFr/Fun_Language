let zero = 
 fn x => 
  fn y => y
in let succ = 
 fn n => 
  fn x => 
   fn y => x (n x y)
in let one = 
 succ zero
in let Y = 
 fn f => (fn x => f (x x)) (fn x => f (x x))
in let f = 
 fn x => one
in let eval = 
 fn n => n (fn a => a + 1) 0
in
 eval (Y f)
