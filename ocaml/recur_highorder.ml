let rec nth l n =
  match l with
    [] -> raise (Failure "list is too short")
    | hd::tl -> match n with
                0 -> hd
                | _ -> nth tl (n-1);;

nth [1; 3; 5; 7] 3;;
nth [1; 2; 3] 1;;

let rec remove_first a l =
  match l with
    [] -> []
    | hd::tl -> if hd = a then tl
                          else hd :: (remove_first a tl);;
                          
remove_first 3 [1; 2; 3; 4; 5];;

let rec insert a l =
  match l with
    [] -> [a]
    | hd::tl -> if a < hd then a::l
                          else hd::(insert a tl);;
                          
insert 2 [1; 3];;

let rec sort l =
  match l with
    [] -> []
    | hd::tl -> insert hd (sort tl);;
  
let rec factorial n =
  if n = 0 then 1 else n * factorial (n-1);;
  
(* factorial 10000000;; overflow *)

let rec inc_all l =
  match l with
  [] -> []
  | h::t -> (h+1)::(inc_all t)
  
let rec square_all l =
  match l with
  [] -> []
  | h::t -> (h*h) :: (square_all t)
  
let rec cube_all l =
  match l with
    [] -> []
    | h::t -> (h*h*h)::(cube_all t);;
    
let rec map f l =
  match l with
    [] -> []
    | h::t -> (f h)::(map f t);;
    
let inc x = x + 1
let square x = x * x
let cube = fun x -> x * x * x;;
let inc_all l = map inc l
let square_all l = map square l
let cube_all l = map cube l;;

map (fun x -> x mod 2 = 1) [1; 2; 3; 4];;

let rec even l =
  match l with
    [] -> []
    | h::t -> if h mod 2 = 0 then h::(even t)
                              else even t;;
                              
let rec greater_than_five l =
  match l with
    [] -> []
    | h::t -> if h>5 then h::(greater_than_five t)
                      else greater_than_five t;;
                      
let rec filter f l =
  match l with
    [] -> []
    | h::t -> if (f h) then h::(filter f t)
                        else (filter f t);;
     
(*                   
let even = filter (fun x -> x mod 2 = 0);;
let gtf = filter (fun x -> x > 5);;

even [1; 2; 3; 4; 5];;
*)

let even = fun x -> x mod 2 = 0
let gtf = fun x -> x > 5;;

filter even [2; 3; 4; 5];;
