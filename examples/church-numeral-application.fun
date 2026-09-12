let x = 
 2
in let f = 
 fn x => 
  fn y => x y
in let succ = 
 fn x => x + 1
in
 f succ 0
