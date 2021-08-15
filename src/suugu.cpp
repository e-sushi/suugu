/* suugu

current brakus naur idea
this is basically the grammar we expect on input
and also helps determine precedence
which we will turn into our own internal format
only basic arithmetic like +-/* and bitwise stuff for now

<input>         :: = <exp>
<exp>           :: = <bitwise or>
<bitwise or>    :: = <bitwise xor> { "|" <bitwise xor> }
<bitwise xor>   :: = <bitwise and> { "^" <bitwise and> }
<bitwise and>   :: = <equality> { "&" <equality> }
<equality>      :: = <relational> { ("!=" | "==") <relational> }
<relational>    :: = <bitwise shift> { ("<" | ">" | "<=" | ">=") <bitwise shift> }
<bitwise shift> :: = <additive> { ("<<" | ">>" ) <additive> }
<additive>      :: = <term> { ("+" | "-") <term> }
<term>          :: = <factor> { ("*" | "/" | "%") <factor> }
<factor>        :: = "(" <exp> ")" | <unary> <factor> | <literal>
<unary>         :: = "!" | "~" | "-"

ideas for parsing itself:
- we only parse when a key is input 
- we only indicate errors when we cant make a tree from what we've parsed
  for ex.
  we do not indicate an error on "3 + " because we are expecting the user to input something
  we do indicate an error on "3 + sin()" because the user missed inputting a literal into the fucntion
  or instead of erroring, we put an empty box where we expect something thats missing
  and only error in cases where the user uses invalid syntax such as "3 <<- 5"
  or if they use an undefined identifier, etc.


TODO Board
----------------------------------------------------

Parser TODOs
------------
- implement a system for adding to an already existing AST tree


*/


//// deshi includes ////
#include "deshi.h"
#include "utils/string.h"
#include "utils/array.h"
#include "utils/map.h"
#include "math/Math.h"

//// STL includes ////


//// suugu headers ////
#include "canvas.h"
#include "syntax.h"
#include "lexer.h"
#include "parser.h"

//// suugu cpps ////
#include "lexer.cpp"
#include "parser.cpp"
#include "solver.cpp"

int main() {
	//suugu vars
    Canvas canvas;
	
    //init deshi
    deshi::init();
	Render::UseDefaultViewProjMatrix();
    
    //init suugu
	canvas.Init();
    
    //start main loop
	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
        
		DeshiImGui::NewFrame();                    //place imgui calls after this
		DeshTime->Update();
		DeshWindow->Update();
		DeshInput->Update();
		DeshConsole->Update(); Console2::Update();
		canvas.Update();
		UI::Update();
		Render::Update();                          //place imgui calls before this
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
    
    //cleanup deshi
	deshi::cleanup();
}