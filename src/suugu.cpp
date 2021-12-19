﻿/* suugu

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
most math should be f64 instead of f32

Parser TODOs
------------
- implement a system for adding to an already existing AST tree


*/


//// deshi includes ////
#include "defines.h"
#include "deshi.h"
#include "utils/string.h"
#include "utils/array.h"
#include "core/memory.h"
#include "utils/map.h"
#include "math/Math.h"
#include "core/logging.h"

//// STL includes ////


//// suugu includes ////
#define SUUGU_IMPLEMENTATION
#include "types.h"
#include "canvas.h"
#include "lexer.cpp"
#include "parser.cpp"
#include "solver.cpp"
#include "canvas.cpp"

int main() {
	//suugu vars
	Canvas canvas;
	
	//init deshi
	deshi::init();
	Render::UseDefaultViewProjMatrix();
	
	
	
	//init suugu
	canvas.Init();

	array<string> images = Assets::iterateDirectory(Assets::dirTextures());
	array<Texture*> textures;
	
	//for (string& str : images) {
	//	textures.add(Storage::CreateTextureFromFile(str.str).second);
	//}



	
	//start main loop
	TIMER_START(t_d); TIMER_START(t_f);
	while (!deshi::shouldClose()) {
		DeshiImGui::NewFrame();                    //place imgui calls after this
		Memory::Update();
		DeshTime->Update();
		DeshWindow->Update();
		DeshInput->Update();
		DeshConsole->Update(); Console2::Update();
		canvas.Update();

		//ImGui::ShowDemoWindow();

		Font* font = Storage::CreateFontFromFileTTF("STIXTwoText-Regular.otf", 400).second;

		{//debug area
			UI::Begin("image", vec2::ONE * 300, vec2::ONE * 300);
			//for (int i = 0; i < textures.count; i += 2) {
			//	UI::BeginRow(2, 170);
			//	UI::RowSetupRelativeColumnWidths({ 1,1 });
			//	UI::SetNextItemSize(vec2(170, 170));
			//	UI::Image(textures[i]);
			//	UI::SetNextItemSize(vec2(170, 170));
			//	UI::Image(textures[i+1]);
			//	UI::EndRow();
			//}
			
			
			static b32 stat[20] = { 0 };
			for (int i = 0; i < 20; i++) {

				if (UI::Selectable(TOSTRING(i, " ABCD").str, stat[i])) {
					stat[i] = !stat[i];
				}
			}
			


			UI::Line(vec2(0, 0), vec2(100, 100));

			
			UI::Text("oh yeah an image baby");
			UI::End();

			UI::Begin("oh yep", vec2::ONE * 300, vec2::ONE * 300);
			static char buff[255] = { 'a', 'b', 'c'};
			for (int i = 0; i < 50; i++) {
				UI::Text(TOSTRING("OH YEAH", i).str);
				UI::Button("test");
				UI::InputText(TOSTRING("label", i, i).str, buff, 255);
			}
			UI::End();


		}
		
		UI::ShowMetricsWindow();
		
		UI::Update();
		Render::Update();                          //place imgui calls before this
		DeshTime->frameTime = TIMER_END(t_f); TIMER_RESET(t_f);
	}
	
	
	
	//cleanup deshi
	deshi::cleanup();
}