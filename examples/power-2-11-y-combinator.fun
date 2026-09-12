let zero = 
 fn x => 
  fn y => y
in let succ = 
 fn n => 
  fn x => 
   fn y => x (n x y)
in let sum = 
 fn u => 
  fn v => 
   fn x => 
    fn y => u x (v x y)
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
in let mult = 
 fn u => 
  fn v => 
   fn x => 
    fn y => u (v x) y
in let church_true = 
 fn x => 
  fn y => x
in let church_false = 
 fn x => 
  fn y => y
in let one = 
 succ zero
in let two = 
 succ (succ zero)
in let three = 
 succ (succ (succ zero))
in let four = 
 succ three
in let is_zero = 
 fn u => u (fn x => church_false) church_true
in let compose_church = 
 fn u => 
  fn v => 
   fn x => 
    fn y => u (v x) y
in let exp_step = 
 fn f => 
  fn n => 
   fn e => is_zero n one (mult e (f (pred n) e))
in let Y = 
 fn g => (fn x => g (x x)) (fn x => g (x x))
in let eval = 
 fn n => n (fn a => a + 1) 0
in let EXP = 
 Y exp_step
in
 eval (EXP (sum three (sum four four)) two)
