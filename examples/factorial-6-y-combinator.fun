let zero = 
 fn x => 
  fn y => y
in let succ = 
 fn n => 
  fn x => 
   fn y => x (n x y)
in let make_pair = 
 fn a => 
  fn b => 
   fn f => f a b
in let snd = 
 fn pair => pair (fn a => 
  fn b => b)
in let fst = 
 fn pair => pair (fn a => 
  fn b => a)
in let next_state = 
 fn pair => make_pair (snd pair) (succ (snd pair))
in let pred = 
 fn n => fst (n next_state (make_pair zero zero))
in let church_true = 
 fn x => 
  fn y => x
in let church_false = 
 fn x => 
  fn y => y
in let if_then_else = 
 fn k => k (fn x => church_false) church_true
in let bool_apply = 
 fn b => 
  fn l => 
   fn r => b l r
in let one = 
 succ zero
in let compose_church = 
 fn u => 
  fn v => 
   fn x => 
    fn y => u (v x) y
in let fact_step = 
 fn f => 
  fn n => bool_apply (if_then_else n) one (compose_church n (f (pred n)))
in let Y = 
 fn g => (fn x => g (x x)) (fn x => g (x x))
in let five = 
 succ (succ (succ (succ (succ (succ zero)))))
in let eval = 
 fn n => n (fn a => a + 1) 0
in
 eval (Y fact_step (succ five))
