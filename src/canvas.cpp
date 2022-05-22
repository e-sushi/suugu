/* Index:
@tools
@binds
@pencil
@camera
@context
@utility
@graph
@canvas
@input
@input_tool
@input_navigation
@input_context
@input_expression
@input_expression_deletion
@input_expression_cursor
@input_expression_insertion
@input_pencil
@draw_elements
@draw_elements_expression
@draw_elements_graph
@draw_elements_workspace
@draw_elements_text
@draw_pencil
@draw_canvas_info
*/

////////////////
//// @tools ////
////////////////
enum CanvasTool : u32{
	CanvasTool_Navigation,
	CanvasTool_Context,
	CanvasTool_Expression,
	CanvasTool_Pencil,
};
local const char* canvas_tool_strings[] = {
	"Navigation", "Context", "Expression", "Pencil",
};

local CanvasTool active_tool   = CanvasTool_Navigation;
local CanvasTool previous_tool = CanvasTool_Navigation;

////////////////
//// @binds ////
////////////////
enum CanvasBind_{ //TODO ideally support multiple keybinds per action
	//[GLOBAL] SetTool
	CanvasBind_SetTool_Navigation = Key_ESCAPE  | InputMod_Any,
	CanvasBind_SetTool_Context    = Mouse_RIGHT | InputMod_AnyCtrl,
	CanvasBind_SetTool_Expression = Key_E       | InputMod_AnyCtrl, //NOTE temp making this CTRL+E for simplicity
	CanvasBind_SetTool_Pencil     = Key_P       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Graph      = Key_G       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Previous   = Mouse_4     | InputMod_None,
	
	//[GLOBAL] Camera 
	CanvasBind_Camera_Pan = Mouse_MIDDLE | InputMod_None,
	
	//[LOCAL]  Navigation 
	CanvasBind_Navigation_Pan       = Mouse_LEFT  | InputMod_Any,
	CanvasBind_Navigation_ResetPos  = Key_NP0     | InputMod_None,
	CanvasBind_Navigation_ResetZoom = Key_NP0     | InputMod_None,
	
	//[LOCAL]  Expression
	CanvasBind_Expression_Select                = Mouse_LEFT    | InputMod_None,
	CanvasBind_Expression_Create                = Mouse_RIGHT   | InputMod_None,
	CanvasBind_Expression_CursorLeft            = Key_LEFT      | InputMod_None,
	CanvasBind_Expression_CursorRight           = Key_RIGHT     | InputMod_None,
	CanvasBind_Expression_CursorWordLeft        = Key_LEFT      | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorWordRight       = Key_RIGHT     | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorUp              = Key_UP        | InputMod_None,
	CanvasBind_Expression_CursorDown            = Key_DOWN      | InputMod_None,
	CanvasBind_Expression_CursorHome            = Key_HOME      | InputMod_None,
	CanvasBind_Expression_CursorEnd             = Key_END       | InputMod_None,
	CanvasBind_Expression_CursorDeleteLeft      = Key_BACKSPACE | InputMod_None,
	CanvasBind_Expression_CursorDeleteRight     = Key_DELETE    | InputMod_None,
	CanvasBind_Expression_CursorDeleteWordLeft  = Key_BACKSPACE | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorDeleteWordRight = Key_DELETE    | InputMod_AnyCtrl,
	
	//[LOCAL]  Pencil
	CanvasBind_Pencil_Stroke             = Mouse_LEFT       | InputMod_Any,
	CanvasBind_Pencil_DeletePrevious     = Key_Z            | InputMod_AnyCtrl,
	CanvasBind_Pencil_DetailIncrementBy1 = Key_EQUALS       | InputMod_None,
	CanvasBind_Pencil_DetailIncrementBy5 = Key_EQUALS       | InputMod_AnyShift,
	CanvasBind_Pencil_DetailDecrementBy1 = Key_MINUS        | InputMod_None,
	CanvasBind_Pencil_DetailDecrementBy5 = Key_MINUS        | InputMod_AnyShift,
}; typedef KeyCode CanvasBind;


/////////////////
//// @pencil ////
/////////////////
struct PencilStroke{
	f32   size;
	color color;
	array<vec2f64> pencil_points;
};

local array<PencilStroke> pencil_strokes;
local u32     pencil_stroke_idx  = 0;
local f32     pencil_stroke_size = 1;
local color   pencil_stroke_color = PackColorU32(249,195,69,255);
local vec2f64 pencil_stroke_start_pos;
local u32     pencil_draw_skip_amount = 4;


/////////////////
//// @camera ////
/////////////////
local vec2f64 camera_pos{0,0};
local f64     camera_zoom = 1.0;
local vec2f64 camera_pan_start_pos;
local vec2    camera_pan_mouse_pos;
local b32     camera_pan_active = false;

local vec2 
ToScreen(vec2f64 point){
	point -= camera_pos;
	point /= camera_zoom;
	point.y *= -f64(DeshWindow->width) / f64(DeshWindow->height);
	point += vec2f64{1.0, 1.0};
	point.x *= f64(DeshWindow->dimensions.x); point.y *= f64(DeshWindow->dimensions.y);
	point /= 2.0;
	return vec2(point.x, point.y);
}FORCE_INLINE vec2 ToScreen(f64 x, f64 y){ return ToScreen({x,y}); }

local vec2f64 
ToWorld(vec2 _point){
	vec2f64 point{_point.x, _point.y};
	point.x /= f64(DeshWindow->dimensions.x); point.y /= f64(DeshWindow->dimensions.y);
	point *= 2.0;
	point -= vec2f64{1.0, 1.0};
	point.y /= -f64(DeshWindow->width) / f64(DeshWindow->height);
	point *= camera_zoom;
	point += camera_pos;
	return point;
}FORCE_INLINE vec2f64 ToWorld(f32 x, f32 y){ return ToWorld({x,y}); }

//returns the width and height of the area in world space that the user can currently see as a vec2
local vec2 
WorldViewArea(){
	return vec2(2 * camera_zoom, 2 * camera_zoom * (float)DeshWindow->height / DeshWindow->width);
}


//////////////////
//// @context ////
//////////////////
local char context_input_buffer[256] = {};
local u32 context_dropdown_selected_index = 0;
local const char* context_dropdown_option_strings[] = {
	"Tool: Navigation", "Tool: Expression", "Tool: Pencil",
	"Add: Graph",
};


////////////////////
//// @draw_term ////
////////////////////
struct DrawContext{
	vec2        bbx; // the bounding box formed by child nodes 
	Vertex2* vstart; // we must save these for the parent node to readjust what the child node makes
	u32*     istart;
	u32 vcount, icount;
};

struct{ //information for the current instance of draw_term, this will need to be its own thing if we ever do multithreading here somehow
	UIItem*      item;
	UIDrawCmd drawCmd;
	vec2 cursor_start;
	f32      cursor_y;
	b32 initialized = false;
}drawinfo;

struct{
	f32  additive_padding = 5;                 //padding between + or - and it's operands
	f32  multiplication_explicit_padding = 3;  //padding between * and it's operands
	f32  multiplication_implicit_padding = 3;  //padding between implicit multiplication operands
	f32  division_padding = 0;                 //padding between division's line and it's operands
	//TODO f32 division_scale = 0.8;           //how much to scale division's operands by 
	f32  division_line_overreach = 3;          //how many pixels of over reach the division line has in both directions
	f32  division_line_thickness = 3;          //how thick the division line is 
	vec2 exponential_offset = vec2(-4,-10);    //offset of exponent
	f32  exponential_scaling = 0.4;            //amount to scale the exponent by
}drawcfg;

//NOTE(sushi): in this setup we are depth-first drawing things and readjusting in parent nodes
//			   this means the memory is organized backwards and when we readjust we just interate from the start of
//			   the drawCmd's vertices to the overall count
//NOTE(sushi): we also dont abide by UIDRAWCMD_MAX_* macros here either, which may cause issues later. this is due 
//             to the setup described above and avoids chunking the data of UIDrawCmds and making readjusting them awkward
//TODO(sushi) remove the expr arg and make it part of drawinfo
DrawContext draw_term(Expression* expr, Term* term){DPZoneScoped;
	using namespace UI;
	DPTracyDynMessage(toStr("initialized: ", drawinfo.initialized));
	if(term == 0) return DrawContext();
	if(!drawinfo.initialized) return DrawContext();
	//initializing internally has some issues so for now drawinfo must be initialized before calling this function
	//if(!drawinfo.initialized){
	//	drawinfo.item = BeginCustomItem();
	//	drawinfo.drawCmd = UIDrawCmd();
	//	drawinfo.initialized = true;
	//	drawinfo.item->position = GetWinCursor();
	//}
	//drawcfg.additive_padding = 10 * (sin(DeshTotalTime/1000)+1)/2;
	//drawcfg.division_padding = 10 * (sin(DeshTotalTime/1000)+1)/2;
	//drawcfg.multiplication_explicit_padding = 10 * (sin(DeshTotalTime/1000)+1)/2;

	UIItem* item       = drawinfo.item; //:)
	UIDrawCmd& drawCmd = drawinfo.drawCmd;
	UIStyle style      = GetStyle();
	DrawContext drawContext;
	drawContext.vcount = 0;

	drawContext.vstart = drawCmd.vertices + u32(drawCmd.counts.x);
	drawContext.istart = drawCmd.indices  + u32(drawCmd.counts.y);
	
	const vec2 textScale = vec2::ONE * style.fontHeight / (f32)style.font->max_height;
	
	//this function checks that the shape we are about to add to the drawcmd does not overrun its buffers
	//if it will we just add the drawcmd to the item and make a new one
	auto check_drawcmd = [&](u32 vcount, u32 icount){
		if(drawCmd.counts.x + vcount > UIDRAWCMD_MAX_VERTICES || drawCmd.counts.y + icount > UIDRAWCMD_MAX_INDICES){
			CustomItem_AddDrawCmd(item, drawCmd);
			drawCmd.vertices = (Vertex2*)memtrealloc(drawCmd.vertices, drawCmd.counts.x*2);
			drawCmd.indices = (u32*)memtrealloc(drawCmd.vertices, drawCmd.counts.y*2);
			drawContext.vstart = drawCmd.vertices;
			drawContext.istart = drawCmd.indices;
		}
	};
	
	switch(term->type){
		case TermType_Expression:{
			Expression* expr = ExpressionFromTerm(term);
			vec2 mmbbx = vec2::ZERO;
			if(term->child_count){
				for_node(term->first_child){
					drawContext = draw_term(expr, it);
					forI(drawContext.vcount){
						(drawContext.vstart + i)->pos.x += mmbbx.x;
					}
					mmbbx.x += drawContext.bbx.x;
					mmbbx.y = Max(mmbbx.y, drawContext.bbx.y);
				}
			}
			else{
				
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				
			}
			//expression is the topmost node so drawing will always be finished when it finishes (i hope)
			item->size = mmbbx;
			CustomItem_AdvanceCursor(item);
			CustomItem_AddDrawCmd(item, drawCmd);
			
			//EndCustomItem();
			//drawinfo.initialized = false;			
			return DrawContext();
		}break;
		
		case TermType_Operator:{
			switch(term->op_type){
				case OpType_Parentheses:{
					str8 syml = str8_lit("(");
					str8 symr = str8_lit(")");
					f32  ratio = 1; //ratio of parenthesis to drawn child nodes over y
					vec2 symsize = CalcTextSize(syml); // i sure hope theres no font with different sizes for these
					drawContext.vstart = drawCmd.vertices + u32(drawCmd.counts.x);
					drawContext.istart = drawCmd.indices  + u32(drawCmd.counts.y);
					if(term->child_count == 1){
						DrawContext ret = draw_term(expr, term->first_child);
						ratio = Max(1, ret.bbx.y / symsize.y);
						forI(ret.vcount){
							(ret.vstart + i)->pos.x += symsize.x;
						}
						drawContext.bbx.x = ret.bbx.x + 2*symsize.x;
						drawContext.bbx.y = Max(symsize.y, ret.bbx.y);
						drawContext.vcount = ret.vcount+8;
						drawContext.icount = ret.icount+12;
						check_drawcmd(8,12);
						CustomItem_DCMakeText(drawCmd, syml, vec2(0, 0), Color_White, vec2(1, ratio) * textScale);
						if(HasFlag(term->flags, TermFlag_LeftParenHasMatchingRightParen)){
							CustomItem_DCMakeText(drawCmd, symr, vec2(symsize.x + ret.bbx.x, 0), Color_White, vec2(1, ratio) * textScale);
						}
						else{
							CustomItem_DCMakeText(drawCmd, symr, vec2(symsize.x + ret.bbx.x, 0), Color_White * 0.3, vec2(1, ratio) * textScale);
						}
					}
					else if(!term->child_count){
						drawContext.bbx.x = symsize.x*2;
						drawContext.bbx.y = symsize.y;
						drawContext.vcount = 8;
						drawContext.icount = 12;
						check_drawcmd(8,12);
						CustomItem_DCMakeText(drawCmd, syml, vec2(0, (drawContext.bbx.y - symsize.y) / 2), Color_White, vec2(1, ratio) * textScale);
						CustomItem_DCMakeText(drawCmd, symr, vec2(symsize.x, (drawContext.bbx.y - symsize.y) / 2), Color_White, vec2(1, ratio) * textScale);
					}
					else{
						Log("", "Parenthesis has more than 1 child");
					}
					return drawContext;
				}break;
				
				case OpType_Exponential:{
					if(term->child_count == 2){
						DrawContext retl = draw_term(expr, term->first_child);
						DrawContext retr = draw_term(expr, term->last_child);
						retr.bbx *= drawcfg.exponential_scaling;
						forI(retr.vcount){
							(retr.vstart + i)->pos.x *= drawcfg.exponential_scaling;
							(retr.vstart + i)->pos.y *= drawcfg.exponential_scaling;
							(retr.vstart + i)->pos.x += retl.bbx.x + drawcfg.exponential_offset.x;
						}
						forI(retl.vcount){
							(retl.vstart + i)->pos.y += retr.bbx.y + drawcfg.exponential_offset.y;
						}
						drawContext.bbx.x = retl.bbx.x + retr.bbx.x + drawcfg.exponential_offset.x;
						drawContext.bbx.y = retl.bbx.y + retr.bbx.y + drawcfg.exponential_offset.y;
						drawContext.vcount = retl.vcount + retr.vcount;
					}
					else if(term->child_count == 1){
						str8 sampchar = STR8("8");
						vec2 size = UI::CalcTextSize(sampchar);
						DrawContext ret = draw_term(expr, term->first_child);
						if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
							forI(ret.vcount){
								(ret.vstart + i)->pos.y += size.y*drawcfg.exponential_scaling + drawcfg.exponential_offset.y;
							}
							check_drawcmd(4,6);
							CustomItem_DCMakeFilledRect(drawCmd, vec2(ret.bbx.x+drawcfg.exponential_offset.x, 0), size*drawcfg.exponential_scaling, Color_DarkGrey);
							drawContext.vcount = ret.vcount + 4;
							drawContext.bbx.x = ret.bbx.x + size.x * drawcfg.exponential_scaling + drawcfg.exponential_offset.x;
							drawContext.bbx.y = ret.bbx.y + size.y * drawcfg.exponential_scaling + drawcfg.exponential_offset.y;
						}
						else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
							Assert(!"The AST should not support placing a ^ when it's not preceeded by a valid term");
						}
					}
				}break;
				
				case OpType_Negation:{
					str8 sym = str8_lit("-");
					vec2 symsize = CalcTextSize(sym);
					if(term->child_count == 1){
						DrawContext ret = draw_term(expr, term->first_child);
						forI(ret.vcount){
							(ret.vstart + i)->pos.x += symsize.x;
						}
						drawContext.bbx.x = ret.bbx.x + symsize.x;
						drawContext.bbx.y = Max(ret.bbx.y, symsize.y);
						drawContext.vcount += ret.vcount + 4;
						check_drawcmd(4,6);
						CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, Color_White, textScale);
					}
					else if(!term->child_count){
						drawContext.bbx = symsize;
						drawContext.vcount = 4;
						check_drawcmd(4,6);
						CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, Color_White, textScale);
					}
					else Assert(!"unary op has more than 1 child");
					return drawContext;
				}break;
				
				case OpType_ImplicitMultiplication:{
					if(term->child_count == 2){
						DrawContext retl = draw_term(expr, term->first_child);
						DrawContext retr = draw_term(expr, term->last_child);
						vec2 refbbx = Max(retl.bbx,retr.bbx);
						forI(retl.vcount){
							(retl.vstart + i)->pos.y += (refbbx.y - retl.bbx.y)/2;
						}
						forI(retr.vcount){
							(retr.vstart + i)->pos.x += retl.bbx.x + drawcfg.multiplication_implicit_padding;
							(retr.vstart + i)->pos.y += (refbbx.y - retr.bbx.y) / 2;
						}
						drawContext.vcount = retl.vcount + retr.vcount;
						drawContext.bbx.x = retl.bbx.x+retr.bbx.x+drawcfg.multiplication_implicit_padding;
						drawContext.bbx.y = Max(retl.bbx.y, retr.bbx.y);
					}
					else{
						Assert(!"please tell me (sushi) if this happens");
					}
					return drawContext;
				}break;
				
				case OpType_ExplicitMultiplication:{
					f32 radius = 4;
					if(term->child_count == 2){
						DrawContext retl = draw_term(expr, term->first_child);
						DrawContext retr = draw_term(expr, term->last_child);
						vec2 refbbx = Max(retl.bbx,retr.bbx);
						forI(retl.vcount){
							(retl.vstart + i)->pos.y += (refbbx.y - retl.bbx.y)/2;
						}
						forI(retr.vcount){
							(retr.vstart + i)->pos.x += retl.bbx.x + 2*radius + drawcfg.multiplication_explicit_padding*2;
							(retr.vstart + i)->pos.y += (refbbx.y - retr.bbx.y) / 2;
						}
						drawContext.vcount = retl.vcount + retr.vcount + render_make_circle_counts(15).x;
						drawContext.bbx.x = retl.bbx.x + retr.bbx.r + 2*radius + 2*drawcfg.multiplication_explicit_padding;
						drawContext.bbx.y = Max(retl.bbx.y, Max(retr.bbx.y, radius*2));
						check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
						CustomItem_DCMakeFilledCircle(drawCmd, vec2(retl.bbx.x+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, Color_White); 
					}
					else if(term->child_count == 1){
						DrawContext ret = draw_term(expr, term->first_child);
						drawContext.bbx = vec2(ret.bbx.x+radius*2+drawcfg.additive_padding*2, Max(radius*2, ret.bbx.y));
						if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
							check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
							CustomItem_DCMakeFilledCircle(drawCmd, vec2(ret.bbx.x+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, Color_White);
						}
						else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
							forI(ret.vcount){
								(ret.vstart + i)->pos.x += radius*2+drawcfg.additive_padding*2;
							}
							check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
							CustomItem_DCMakeFilledCircle(drawCmd, vec2(radius+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, Color_White);
						}
						drawContext.vcount = ret.vcount + render_make_circle_counts(15).x;
					}
					else if(!term->child_count){
						drawContext.bbx = vec2(style.fontHeight*style.font->aspect_ratio,style.fontHeight);
						check_drawcmd(render_make_circle_counts(15).x,render_make_circle_counts(15).y);
						drawContext.vcount = render_make_circle_counts(15).x;
						CustomItem_DCMakeFilledCircle(drawCmd, vec2(radius+drawcfg.multiplication_explicit_padding, drawContext.bbx.y/2), radius, 15, Color_White);
					}
					return drawContext;
				}break;
				
				case OpType_Division:{
					if(term->child_count == 2){
						DrawContext retl = draw_term(expr, term->first_child);
						DrawContext retr = draw_term(expr, term->last_child);
						vec2 refbbx = Max(retl.bbx, retr.bbx);
						refbbx.x += drawcfg.division_line_overreach*2;
						for(Vertex2* v = retl.vstart; v != retr.vstart; v++){
							v->pos.x += (refbbx.x - retl.bbx.x) / 2; 
						}
						forI(retr.vcount){
							(retr.vstart + i)->pos.x += (refbbx.x - retr.bbx.x) / 2;
							(retr.vstart + i)->pos.y += retl.bbx.y + drawcfg.division_padding;
						}
						drawContext.vcount = retl.vcount + retr.vcount + 4;
						drawContext.bbx = vec2(refbbx.x, retl.bbx.y+drawcfg.division_padding+retr.bbx.y);
						check_drawcmd(4,6);
						CustomItem_DCMakeLine(drawCmd, vec2(0, retl.bbx.y+(drawcfg.division_padding-drawcfg.division_line_thickness/2)),  vec2(drawContext.bbx.x, retl.bbx.y+(drawcfg.division_padding-drawcfg.division_line_thickness/2)), drawcfg.division_line_thickness, Color_White);
					}
					else if(term->child_count == 1){
						DrawContext ret = draw_term(expr, term->first_child);
						drawContext.bbx = vec2(ret.bbx.x+drawcfg.division_line_overreach*2, ret.bbx.y*2+drawcfg.division_padding);
						if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
							forI(ret.vcount){
								(ret.vstart + i)->pos.x += (drawContext.bbx.x-ret.bbx.x)/2;
							}
							check_drawcmd(4,6);
							CustomItem_DCMakeLine(drawCmd, vec2(0, (drawContext.bbx.y-drawcfg.division_line_thickness)/2),vec2(drawContext.bbx.x, (drawContext.bbx.y-drawcfg.division_line_thickness)/2), drawcfg.division_line_thickness, Color_White);
						}
						else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
							forI(ret.vcount){
								(ret.vstart + i)->pos.x += (drawContext.bbx.x-ret.bbx.x)/2;
								(ret.vstart + i)->pos.y += drawContext.bbx.y-ret.bbx.y;
							}
							check_drawcmd(4,6);
							CustomItem_DCMakeLine(drawCmd, vec2(0, (drawContext.bbx.y-drawcfg.division_line_thickness)/2),vec2(drawContext.bbx.x, (drawContext.bbx.y-drawcfg.division_line_thickness)/2), drawcfg.division_line_thickness, Color_White);
							//CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, Color_White, textScale);
						}
						drawContext.vcount = ret.vcount + 4;
						drawContext.icount = ret.icount + 6;
					}
					else if(!term->child_count){
						drawContext.bbx = vec2(style.fontHeight*style.font->aspect_ratio+2*drawcfg.division_line_overreach, style.fontHeight*2+drawcfg.division_padding);
						drawContext.vcount = 4;
						drawContext.icount = 6;
						check_drawcmd(4,6);
						CustomItem_DCMakeLine(drawCmd, vec2(0, drawContext.bbx.y/2),vec2(drawContext.bbx.x, drawContext.bbx.y/2), 1, Color_White);
					}
					return drawContext;
				}break;
				
				case OpType_Modulo:{
					
				}break;
				
				case OpType_Addition:
				case OpType_Subtraction:{
					str8 sym;
					if(term->op_type == OpType_Addition)         sym = str8_lit("+");
					else if(term->op_type == OpType_Subtraction) sym = str8_lit("−");
					vec2 symsize = CalcTextSize(sym);
					
					//this can maybe be a switch
					//both children exist so proceed normally
					if(term->child_count == 2){
						DrawContext retl = draw_term(expr, term->first_child);
						DrawContext retr = draw_term(expr, term->last_child);
						vec2 refbbx = Max(retl.bbx,retr.bbx);
						forI(retl.vcount){
							(retl.vstart + i)->pos.y += (refbbx.y - retl.bbx.y)/2;
						}
						forI(retr.vcount){
							(retr.vstart + i)->pos.x += retl.bbx.x + symsize.x + drawcfg.additive_padding*2;
							(retr.vstart + i)->pos.y += (refbbx.y - retr.bbx.y) / 2;
						}
						drawContext.bbx = vec2(retl.bbx.x+retr.bbx.x+symsize.x+drawcfg.additive_padding*2, Max(retl.bbx.y, Max(retr.bbx.y, symsize.y)));
						drawContext.vcount = retl.vcount + retr.vcount + 4;
						drawContext.icount = retl.icount + retr.icount + 6;
						//DrawDebugRect(GetWindow()->position + item->position, dcFirst.bbx);
						check_drawcmd(4,6);
						CustomItem_DCMakeText(drawCmd, sym, vec2(retl.bbx.x+drawcfg.additive_padding, (drawContext.bbx.y - symsize.y)/2), Color_White, textScale);
					}
					//operator has a first child but it isnt followed by anything
					else if(term->child_count == 1){
						DrawContext ret = draw_term(expr, term->first_child);
						drawContext.bbx = vec2(ret.bbx.x+symsize.x+drawcfg.additive_padding*2, Max(symsize.y, ret.bbx.y));
						if(HasFlag(term->first_child->flags, TermFlag_OpArgLeft)){
							check_drawcmd(4,6);
							CustomItem_DCMakeText(drawCmd, sym, vec2(ret.bbx.x+drawcfg.additive_padding, (drawContext.bbx.y - symsize.y)/2), Color_White, textScale);
						}
						else if(HasFlag(term->first_child->flags, TermFlag_OpArgRight)){
							forI(ret.vcount){
								(ret.vstart + i)->pos.x += symsize.x+drawcfg.additive_padding*2;
							}
							check_drawcmd(4,6);
							CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, Color_White, textScale);
						}
						drawContext.vcount = ret.vcount + 4;
						drawContext.icount = ret.icount + 6;
					}
					else if(!term->child_count){
						drawContext.bbx = symsize;
						drawContext.vcount = 4;
						drawContext.icount = 6;
						check_drawcmd(4,6);
						CustomItem_DCMakeText(drawCmd, sym, vec2::ZERO, Color_White, textScale);
					}
					else Assert(!"binop has more than 2 children");
					return drawContext;
				}break;
				
				case OpType_ExpressionEquals:{
					
				}break;
				
				default: Assert(!"operator type drawing not setup"); break;
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				
			}
		}break;
		
		case TermType_Literal:
		case TermType_Variable:{
			drawContext.bbx = CalcTextSize(term->raw);
			drawContext.vcount = str8_length(term->raw) * 4;
			drawContext.icount = str8_length(term->raw) * 6;
			check_drawcmd(drawContext.vcount,drawContext.icount);
			CustomItem_DCMakeText(drawCmd, term->raw, vec2::ZERO, Color_White, textScale);
			return drawContext;
		}break;
		
		case TermType_FunctionCall:{
			//TODO(sushi) support multi arg functions when implemented
			if(term->first_child){
				DrawContext ret = draw_term(expr, term->first_child);
				drawContext.vcount = str8_length(term->raw) * 4 + ret.vcount;
				drawContext.icount = str8_length(term->raw) * 6 + ret.icount;
				check_drawcmd(drawContext.vcount, drawContext.icount);
				vec2 tsize = UI::CalcTextSize(term->raw);
				forI(ret.vcount){
					(ret.vstart + i)->pos.x += tsize.x;
				}
				CustomItem_DCMakeText(drawCmd, term->raw, vec2(0, (ret.bbx.y-tsize.y)/2), Color_White, textScale);
				drawContext.bbx.x = tsize.x + ret.bbx.x;
				drawContext.bbx.y = Max(tsize.y, ret.bbx.y);
			}else{
				Assert(!"huh?");
			}
			return drawContext;
		}break;
		
		case TermType_Logarithm:{
			
		}break;
		
		default: LogE("exrend", "Custom rendering does not support term type:", OpTypeStrs(term->type)); break;//Assert(!"term type drawing not setup"); break;
	}
	
	
	return drawContext;
}

void draw_term_old(Expression* expr, Term* term, vec2& cursor_start, f32& cursor_y){
	if(term == 0) return;
	switch(term->type){
		case TermType_Expression:{
			Expression* expr = ExpressionFromTerm(term);
			if(term->child_count){
				for_node(term->first_child){
					UI::Text(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
					draw_term_old(expr, it, cursor_start, cursor_y);
				}
			}else{
				UI::Text(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				UI::Text(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
				if(expr->cursor_start == 1){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
			}
			if(expr->cursor_start == expr->raw.count){ cursor_start = UI::GetLastItemPos() + UI::GetLastItemSize(); cursor_y = -UI::GetLastItemSize().y; }
			
			//draw solution if its valid
			if(expr->valid){
				UI::PushColor(UIStyleCol_Text, Color_Grey);
				if(expr->equals){
					if(expr->solution == MAX_F64){
						UI::Text(str8_lit("ERROR"));
					}else{
						UI::TextF(str8_lit("%g"), expr->solution);
					}
					UI::SameLine();
				}else if(expr->solution != MAX_F64){
					UI::Text(str8_lit("="), UITextFlags_NoWrap); UI::SameLine();
					UI::TextF(str8_lit("%g"), expr->solution);
					UI::SameLine();
				}
				UI::PopColor();
			}
			UI::Text(str8_lit(" "), UITextFlags_NoWrap);
		}break;
		
		case TermType_Operator:{
			switch(term->op_type){
				case OpType_Parentheses:{
					UI::Text(str8_lit("("), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->first_child){
						draw_term_old(expr, term->first_child, cursor_start, cursor_y);
						for_node(term->first_child->next){
							UI::Text(str8_lit(" "), UITextFlags_NoWrap); UI::SameLine();
							draw_term_old(expr, it, cursor_start, cursor_y);
						}
					}
					if(HasFlag(term->flags, TermFlag_LeftParenHasMatchingRightParen)){
						UI::Text(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
						if(expr->raw.str + expr->cursor_start == term->raw.str + term->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					}
				}break;
				
				case OpType_Exponential:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					UI::Text(str8_lit("^"), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
				}break;
				
				case OpType_Negation:{
					UI::Text(str8_lit("-"), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					draw_term_old(expr, term->first_child, cursor_start, cursor_y);
				}break;
				
				case OpType_ImplicitMultiplication:{
					draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					draw_term_old(expr, term->last_child, cursor_start, cursor_y);
				}break;

				case OpType_ExplicitMultiplication:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					UI::Text(str8_lit("*"), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
				}break;
				case OpType_Division:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					UI::Text(str8_lit("/"), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
				}break;
				case OpType_Modulo:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					UI::Text(str8_lit("%"), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
				}break;
				
				case OpType_Addition:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					UI::Text(str8_lit("+"), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
				}break;
				case OpType_Subtraction:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					UI::Text(str8_lit("-"), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
				}break;
				
				case OpType_ExpressionEquals:{
					if(term->first_child && HasFlag(term->first_child->flags, TermFlag_OpArgLeft)) draw_term_old(expr, term->first_child, cursor_start, cursor_y);
					UI::Text(str8_lit("="), UITextFlags_NoWrap); UI::SameLine();
					if(expr->raw.str + expr->cursor_start == term->raw.str){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
					if(term->last_child && HasFlag(term->last_child->flags, TermFlag_OpArgRight)) draw_term_old(expr, term->last_child, cursor_start, cursor_y);
					if(term->last_child) for_node(term->last_child->next) draw_term_old(expr, it, cursor_start, cursor_y);
				}break;
				
				default: Assert(!"operator type drawing not setup"); break;
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				UI::Text(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
				if(expr->raw.str + expr->cursor_start == term->raw.str + term->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
			}
		}break;
		
		case TermType_Literal:
		case TermType_Variable:{
			//TODO italics for variables (make this an option)
			UI::Text(str8{(u8*)term->raw.str, (s64)term->raw.count}, UITextFlags_NoWrap); UI::SameLine();
			if((term->raw.str <= expr->raw.str + expr->cursor_start) && (expr->raw.str + expr->cursor_start < term->raw.str + term->raw.count)){
				f32 x_offset = UI::CalcTextSize(str8{(u8*)term->raw.str, s64(expr->raw.str + expr->cursor_start - term->raw.str)}).x;
				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
			}
			if(HasFlag(term->flags, TermFlag_DanglingClosingParenToRight)){
				UI::Text(str8_lit(")"), UITextFlags_NoWrap); UI::SameLine();
				if(expr->raw.str + expr->cursor_start == term->raw.str + term->raw.count){ cursor_start = UI::GetLastItemPos(); cursor_y = UI::GetLastItemSize().y; }
			}
		}break;
		
		case TermType_FunctionCall:{
			UI::Text(str8{(u8*)term->raw.str, (s64)term->raw.count}, UITextFlags_NoWrap); UI::SameLine();
			if((term->raw.str <= expr->raw.str + expr->cursor_start) && (expr->raw.str + expr->cursor_start < term->raw.str + term->raw.count)){
				f32 x_offset = UI::CalcTextSize(str8{(u8*)term->raw.str, s64(expr->raw.str + expr->cursor_start - term->raw.str)}).x;
				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
			}
			for_node(term->first_child) draw_term_old(expr, it, cursor_start, cursor_y);
		}break;
		
		case TermType_Logarithm:{
			UI::Text(str8{(u8*)term->raw.str, (s64)term->raw.count}, UITextFlags_NoWrap); UI::SameLine();
			if((term->raw.str <= expr->raw.str + expr->cursor_start) && (expr->raw.str + expr->cursor_start < term->raw.str + term->raw.count)){
				f32 x_offset = UI::CalcTextSize(str8{(u8*)term->raw.str, s64(expr->raw.str + expr->cursor_start - term->raw.str)}).x;
				cursor_start = UI::GetLastItemPos() + vec2{x_offset,0}; cursor_y = UI::GetLastItemSize().y;
			}
			for_node(term->first_child) draw_term_old(expr, it, cursor_start, cursor_y);
		}break;
		
		default: Assert(!"term type drawing not setup"); break;
	}
	
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @canvas
local array<Element*> elements(deshi_allocator);
local Element* selected_element;
local GraphElement* activeGraph; //TODO remove this and use selected_element instead
local vec2f64 mouse_pos_world;
local Font* math_font;
local GraphElement defgraph; //temp default graph

void init_canvas(){
	f32 world_height  = WorldViewArea().y;
	defgraph.element.height = world_height / 2.f;
	defgraph.element.width  = world_height / 2.f;
	defgraph.element.y      =  defgraph.element.height / 2.f;
	defgraph.element.x      = -defgraph.element.width  / 2.f;
	defgraph.element.type   = ElementType_Graph;
	defgraph.graph = (Graph*)memory_alloc(sizeof(Graph));
	Graph g; memcpy(defgraph.graph, &g, sizeof(Graph));
	defgraph.graph->xAxisLabel = str8_lit("x");
	defgraph.graph->yAxisLabel = str8_lit("y");
	elements.add(&defgraph.element);
	
	//load_constants();
	
	math_font = Storage::CreateFontFromFileTTF(str8_lit("STIXTwoMath-Regular.otf"), 100).second;
	Assert(math_font != Storage::NullFont(), "Canvas math font failed to load");
}

void update_canvas(){
	UI::PushVar(UIStyleVar_WindowMargins, vec2::ZERO);
	UI::SetNextWindowSize(DeshWindow->dimensions);
	UI::Begin(str8_lit("canvas"), vec2::ZERO, vec2::ZERO, UIWindowFlags_Invisible | UIWindowFlags_NoInteract);
	
	//-///////////////////////////////////////////////////////////////////////////////////////////////
	//// @input
	mouse_pos_world = ToWorld(input_mouse_position());
	
	//// @input_tool ////
	if      (key_pressed(CanvasBind_SetTool_Navigation) && active_tool != CanvasTool_Navigation){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Navigation;
	}else if(key_pressed(CanvasBind_SetTool_Context)    && active_tool != CanvasTool_Context){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Context;
	}else if(key_pressed(CanvasBind_SetTool_Expression) && active_tool != CanvasTool_Expression){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Expression;
	}else if(key_pressed(CanvasBind_SetTool_Pencil)     && active_tool != CanvasTool_Pencil){
		previous_tool = active_tool;
		active_tool   = CanvasTool_Pencil;
	}else if(key_pressed(CanvasBind_SetTool_Graph)){
		activeGraph   = (activeGraph) ? 0 : &defgraph;
	}else if(key_pressed(CanvasBind_SetTool_Previous)){
		Swap(previous_tool, active_tool);
	}
	
	//// @input_camera ////
	if(key_pressed(CanvasBind_Camera_Pan)){
		camera_pan_active = true;
		camera_pan_mouse_pos = input_mouse_position();
		camera_pan_start_pos = camera_pos;
	}
	if(key_down(CanvasBind_Camera_Pan)){
		camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
	}
	if(key_released(CanvasBind_Camera_Pan)){
		camera_pan_active = false;
	}
	if(DeshInput->scrollY != 0 && input_mods_down(InputMod_None) && !UI::AnyWinHovered()){
		if(!activeGraph){
			camera_zoom -= camera_zoom / 10.0 * DeshInput->scrollY;
			camera_zoom = Clamp(camera_zoom, 1e-37, 1e37);
		}else{
			activeGraph->graph->cameraZoom -= 0.2*activeGraph->graph->cameraZoom*DeshInput->scrollY;
		}
	}
	
#if 1 //NOTE temp ui
	if(active_tool == CanvasTool_Pencil){
		UI::Begin(str8_lit("pencil_debug"), {200,10}, {200,200}, UIWindowFlags_FitAllElements);
		UI::TextF(str8_lit("Stroke Size:   %f"), pencil_stroke_size);
		UI::TextF(str8_lit("Stroke Color:  %x"), pencil_stroke_color.rgba);
		UI::TextF(str8_lit("Stroke Start:  (%g,%g)"), pencil_stroke_start_pos.x, pencil_stroke_start_pos.y);
		UI::TextF(str8_lit("Stroke Index:  %d"), pencil_stroke_idx);
		UI::TextF(str8_lit("Stroke Skip:   %d"), pencil_draw_skip_amount);
		if(pencil_stroke_idx > 0) UI::TextF(str8_lit("Stroke Points: %d"), pencil_strokes[pencil_stroke_idx-1].pencil_points.count);
		u32 total_stroke_points = 0;
		forE(pencil_strokes) total_stroke_points += it->pencil_points.count;
		UI::TextF(str8_lit("Total Points:  %d"), total_stroke_points);
		UI::End();
	}
	if(active_tool == CanvasTool_Expression){
		UI::Begin(str8_lit("expression_debug"), {200,10}, {200,200}, UIWindowFlags_FitAllElements);
		UI::TextF(str8_lit("Elements: %d"), elements.count);
		if(selected_element){
			UI::TextF(str8_lit("Selected: %#x"), selected_element);
			UI::TextF(str8_lit("Position: (%g,%g)"), selected_element->x,selected_element->y);
			UI::TextF(str8_lit("Size:     (%g,%g)"), selected_element->width,selected_element->height);
			UI::TextF(str8_lit("Cursor:   %d"), (selected_element) ? ((Expression*)selected_element)->cursor_start : 0);
		}
		UI::End();
	}
	if(activeGraph){
		UI::Begin(str8_lit("graph_debug"), {200,10}, {200,200}, UIWindowFlags_FitAllElements);
		UI::Text(str8_lit("Graph Info"));
		UI::TextF(str8_lit("Element Pos:   (%g,%g)"), activeGraph->element.x,activeGraph->element.y);
		UI::TextF(str8_lit("Element Size:  (%g,%g)"), activeGraph->element.width,activeGraph->element.height);
		UI::TextF(str8_lit("Camera Pos:    (%g,%g)"), activeGraph->graph->cameraPosition.x,activeGraph->graph->cameraPosition.y);
		UI::TextF(str8_lit("Camera View:   (%g,%g)"), activeGraph->graph->cameraZoom);
		UI::TextF(str8_lit("Camera Zoom:   %g"), activeGraph->graph->cameraView.x,activeGraph->graph->cameraView.y);
		UI::TextF(str8_lit("Dims per Unit: (%g,%g)"), activeGraph->graph->dimensions_per_unit_length.x, activeGraph->graph->dimensions_per_unit_length.y);
		UI::TextF(str8_lit("Aspect Ratio:  %g"), activeGraph->graph->aspect_ratio);
		UI::End();
	}
#endif
	
	switch(active_tool){
		//// @input_navigation ////
		case CanvasTool_Navigation: if(!UI::AnyWinHovered()){
			if(key_pressed(CanvasBind_Navigation_Pan)){
				camera_pan_active = true;
				camera_pan_mouse_pos = input_mouse_position();
				if(!activeGraph){
					camera_pan_start_pos = camera_pos;
				}else{
					camera_pan_start_pos = vec2f64{activeGraph->graph->cameraPosition.x, activeGraph->graph->cameraPosition.y};
				}
			}
			if(key_down(CanvasBind_Navigation_Pan)){
				if(!activeGraph){
					camera_pos = camera_pan_start_pos + (ToWorld(camera_pan_mouse_pos) - mouse_pos_world);
				}else{
					activeGraph->graph->cameraPosition.x = (camera_pan_start_pos.x + (camera_pan_mouse_pos.x - DeshInput->mouseX) / activeGraph->graph->dimensions_per_unit_length.x);
					activeGraph->graph->cameraPosition.y = (camera_pan_start_pos.y + (camera_pan_mouse_pos.y - DeshInput->mouseY) / activeGraph->graph->dimensions_per_unit_length.y);
				}
			}
			if(key_released(CanvasBind_Navigation_Pan)){
				camera_pan_active = false;
			}
			if(key_pressed(CanvasBind_Navigation_ResetPos)){
				if(!activeGraph){
					camera_pos = {0,0};
				}else{
					activeGraph->graph->cameraPosition = {0,0};
				}
			}
			if(key_pressed(CanvasBind_Navigation_ResetZoom)){
				if(!activeGraph){
					camera_zoom = 1.0;
				}else{
					activeGraph->graph->cameraZoom = 5.0;
				}
			}
		}break;
		
		//// @input_context ////
		case CanvasTool_Context:{
			//if(UI::BeginContextMenu("canvas_context_menu")){
			//UI::EndContextMenu();
			//}
		}break;
		
		//// @input_expression ////
		case CanvasTool_Expression: if(!UI::AnyWinHovered()){
			if(key_pressed(CanvasBind_Expression_Select)){
				selected_element = 0;
				//TODO inverse the space transformation here since mouse pos is screen space, which is less precise being
				//  elevated to higher precision, instead of higher precision world space getting transformed to screen space
				for(Element* it : elements){
					if(   mouse_pos_world.x >= it->x
					   && mouse_pos_world.y >= it->y
					   && mouse_pos_world.x <= it->x + it->width
					   && mouse_pos_world.y <= it->y + it->height){
						selected_element = it;
						break;
					}
				}
			}
			
			if(key_pressed(CanvasBind_Expression_Create)){
				Expression* expr = (Expression*)memory_alloc(sizeof(Expression)); //TODO expression arena
				expr->element.x       = mouse_pos_world.x;
				expr->element.y       = mouse_pos_world.y;
				expr->element.height  = (320*camera_zoom) / (f32)DeshWindow->width;
				expr->element.width   = expr->element.height / 2.0;
				expr->element.type    = ElementType_Expression;
				expr->term.type       = TermType_Expression;
				expr->terms.allocator = deshi_allocator;
				expr->cursor_start    = 1;
				str8_builder_init(&expr->raw, str8_lit(" "), deshi_allocator);
				
				elements.add(&expr->element);
				selected_element = &expr->element;
			}
			
			if(selected_element && selected_element->type == ElementType_Expression){
				Expression* expr = ElementToExpression(selected_element);
				b32 ast_changed = false;
				
				//// @input_expression_cursor ////
				if(expr->cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorLeft)){
					expr->cursor_start -= 1;
				}
				if(expr->cursor_start < expr->raw.count && key_pressed(CanvasBind_Expression_CursorRight)){
					expr->cursor_start += 1;
				}
				if(expr->cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorWordLeft)){
					if(*(expr->raw.str+expr->cursor_start-1) == ')'){
						while(expr->cursor_start > 1 && *(expr->raw.str+expr->cursor_start-1) != '('){
							expr->cursor_start -= 1;
						}
						if(*(expr->raw.str+expr->cursor_start-1) == '(') expr->cursor_start -= 1;
					}else if(ispunct(*(expr->raw.str+expr->cursor_start-1)) && *(expr->raw.str+expr->cursor_start-1) != '.'){
						expr->cursor_start -= 1;
					}else{
						while(expr->cursor_start > 1 && (isalnum(*(expr->raw.str+expr->cursor_start-1)) || *(expr->raw.str+expr->cursor_start-1) == '.')){
							expr->cursor_start -= 1;
						}
					}
				}
				if(expr->cursor_start < expr->raw.count && key_pressed(CanvasBind_Expression_CursorWordRight)){
					if(*(expr->raw.str+expr->cursor_start) == '('){
						while(expr->cursor_start < expr->raw.count && *(expr->raw.str+expr->cursor_start) != ')'){
							expr->cursor_start += 1;
						}
						if(*(expr->raw.str+expr->cursor_start) == ')') expr->cursor_start += 1;
					}else if(ispunct(*(expr->raw.str+expr->cursor_start)) && *(expr->raw.str+expr->cursor_start) != '.'){
						expr->cursor_start += 1;
					}else{
						while(expr->cursor_start < expr->raw.count && (isalnum(*(expr->raw.str+expr->cursor_start)) || *(expr->raw.str+expr->cursor_start) == '.')){
							expr->cursor_start += 1;
						}
					}
				}
				if(key_pressed(CanvasBind_Expression_CursorHome)){
					expr->cursor_start = 1;
				}
				if(key_pressed(CanvasBind_Expression_CursorEnd)){
					expr->cursor_start = expr->raw.count;
				}
				
				//character based input (letters, numbers, symbols)
				//// @input_expression_insertion ////
				if(DeshInput->charCount){
					ast_changed = true;
					str8_builder_insert_byteoffset(&expr->raw, expr->cursor_start, str8{DeshInput->charIn, DeshInput->charCount});
					expr->cursor_start += DeshInput->charCount;
				}
				
				//// @input_expression_deletion ////
				if(expr->cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorDeleteLeft)){
					ast_changed = true;
					str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start-1);
					expr->cursor_start -= 1;
				}
				if(expr->cursor_start < expr->raw.count-1 && key_pressed(CanvasBind_Expression_CursorDeleteRight)){
					ast_changed = true;
					str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start);
				}
				if(expr->cursor_start > 1 && key_pressed(CanvasBind_Expression_CursorDeleteWordLeft)){
					ast_changed = true;
					if(*(expr->raw.str+expr->cursor_start-1) == ')'){
						while(expr->cursor_start > 1 && *(expr->raw.str+expr->cursor_start-1) != '('){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start-1);
							expr->cursor_start -= 1;
						}
						if(*(expr->raw.str+expr->cursor_start-1) == '('){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start-1);
							expr->cursor_start -= 1;
						}
					}else if(ispunct(*(expr->raw.str+expr->cursor_start-1)) && *(expr->raw.str+expr->cursor_start-1) != '.'){
						str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start-1);
						expr->cursor_start -= 1;
					}else{
						while(expr->cursor_start > 1 && (isalnum(*(expr->raw.str+expr->cursor_start-1)) || *(expr->raw.str+expr->cursor_start-1) == '.')){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start-1);
							expr->cursor_start -= 1;
						}
					}
				}
				if(expr->cursor_start < expr->raw.count-1 && key_pressed(CanvasBind_Expression_CursorDeleteWordRight)){
					ast_changed = true;
					if(*(expr->raw.str+expr->cursor_start) == '('){
						while(expr->cursor_start < expr->raw.count && *(expr->raw.str+expr->cursor_start) != ')'){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start);
						}
						if(*(expr->raw.str+expr->cursor_start) == ')') str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start);
					}else if(ispunct(*(expr->raw.str+expr->cursor_start)) && *(expr->raw.str+expr->cursor_start) != '.'){
						str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start);
					}else{
						while(expr->cursor_start < expr->raw.count && (isalnum(*(expr->raw.str+expr->cursor_start)) || *(expr->raw.str+expr->cursor_start) == '.')){
							str8_builder_remove_codepoint_at_byteoffset(&expr->raw, expr->cursor_start);
						}
					}
				}
				
				if(ast_changed){
					expr->valid = parse(expr);
					solve(&expr->term);
					debug_print_term(&expr->term);
				}
			}
		}break;
		
		////////////////////////////////////////////////////////////////////////////////////////////////
		//// @input_pencil
		case CanvasTool_Pencil: if(!UI::AnyWinHovered()){
			if(key_pressed(CanvasBind_Pencil_Stroke)){
				PencilStroke new_stroke;
				new_stroke.size  = pencil_stroke_size;
				new_stroke.color = pencil_stroke_color;
				pencil_strokes.add(new_stroke);
				pencil_stroke_start_pos = mouse_pos_world;
			}
			if(key_down(CanvasBind_Pencil_Stroke)){
				pencil_strokes[pencil_stroke_idx].pencil_points.add(mouse_pos_world);
			}
			if(key_released(CanvasBind_Pencil_Stroke)){
				pencil_stroke_idx += 1;
			}
			if(key_pressed(CanvasBind_Pencil_DeletePrevious)){ 
				if(pencil_strokes.count){
					pencil_strokes.pop();
					if(pencil_stroke_idx) pencil_stroke_idx -= 1;
				}
			}
			if     (DeshInput->scrollY > 0 && key_down(Key_LSHIFT)){ pencil_stroke_size += 1; }
			else if(DeshInput->scrollY > 0 && key_down(Key_LCTRL)) { pencil_stroke_size += 5; }
			else if(DeshInput->scrollY < 0 && key_down(Key_LSHIFT)){ pencil_stroke_size -= 1; }
			else if(DeshInput->scrollY < 0 && key_down(Key_LCTRL)) { pencil_stroke_size -= 5; }
			pencil_stroke_size = ((pencil_stroke_size < 1) ? 1 : ((pencil_stroke_size > 100) ? 100 : (pencil_stroke_size)));
			if     (key_pressed(CanvasBind_Pencil_DetailIncrementBy1)){ pencil_draw_skip_amount -= 1; }
			else if(key_pressed(CanvasBind_Pencil_DetailIncrementBy5)){ pencil_draw_skip_amount -= 5; }
			else if(key_pressed(CanvasBind_Pencil_DetailDecrementBy1)){ pencil_draw_skip_amount += 1; }
			else if(key_pressed(CanvasBind_Pencil_DetailDecrementBy5)){ pencil_draw_skip_amount += 5; }
			pencil_draw_skip_amount = Clamp(pencil_draw_skip_amount, 1, 100);
		}break;
	}
	
	//// @draw_elements ////
	for(Element* el : elements){
		switch(el->type){
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_expression
			case ElementType_Expression:{
				UI::PushColor(UIStyleCol_Border, (el == selected_element) ? Color_Yellow : Color_White);
				UI::PushVar(UIStyleVar_FontHeight,       80);
				UI::PushVar(UIStyleVar_WindowMargins,    vec2{5,5});
				UI::PushScale(vec2::ONE * el->height / camera_zoom * 2.0);
				UI::PushFont(math_font);
				UI::SetNextWindowPos(ToScreen(el->x, el->y));
				UI::SetNextWindowSize(vec2(el->width,el->height) * f32(DeshWindow->width) / (4 * el->y));
				string s = stringf(deshi_temp_allocator, "expression_0x%p",el);
				UI::Begin(str8{(u8*)s.str, (s64)s.count}, vec2::ZERO, vec2::ZERO, UIWindowFlags_NoInteract | UIWindowFlags_FitAllElements);
				
				Expression* expr = ElementToExpression(el);
				vec2 cursor_start; f32 cursor_y;
				//NOTE(sushi): drawinfo initialization is temporarily done outside the draw_term function and ideally will be added back later
				//             or we make a drawinfo struct and pass it in to (possibly) support parallelizing this
				drawinfo.drawCmd = UIDrawCmd();
				drawinfo.initialized = true;
				static b32 tog = 0;
				if(tog) draw_term_old(expr, &expr->term, cursor_start, cursor_y);
				else{
					drawinfo.item = UI::BeginCustomItem();
					draw_term(expr, &expr->term);
					UI::EndCustomItem();
				}	
				drawinfo.initialized = false;
				if(key_pressed(Key_UP)) ToggleBool(tog);
				//if(expr->raw.str){
				//	UI::SetNextItemActive();
				//	UI::InputText("textrenderdebugdisplay", expr->raw.str, expr->raw.count, 0, UIInputTextFlags_FitSizeToText | UIInputTextFlags_NoEdit);
				//	UI::GetInputTextState("textrenderdebugdisplay")->cursor = expr->cursor_start;
				//}
				if(selected_element == el){
					UI::Line(cursor_start, cursor_start + vec2{0,cursor_y}, 2, Color_White * abs(sin(DeshTime->totalTime)));
				}

				
				UI::End();
				UI::PopFont();
				UI::PopScale();
				UI::PopVar(2);
				UI::PopColor();
			}break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_graph
			case ElementType_Graph:{
				GraphElement* ge = ElementToGraphElement(el);
				vec2 tl = ToScreen(ge->element.x, ge->element.y);
				vec2 br = ToScreen(ge->element.x + ge->element.width, ge->element.y - ge->element.height);
				draw_graph(ge->graph, vec2g{tl.x, tl.y}, vec2g{br.x - tl.x, br.y - tl.y});
			}break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_workspace
			//case ElementType_Workspace:{}break;
			
			///////////////////////////////////////////////////////////////////////////////////////////////
			//// @draw_elements_text
			//case ElementType_Text:{}break;
			
			default:{
				NotImplemented;
			}break;
		}
	}
	
	//// @draw_pencil //// //TODO smooth line drawing
	UI::Begin(str8_lit("pencil_layer"), vec2::ZERO, DeshWindow->dimensions, UIWindowFlags_Invisible | UIWindowFlags_NoInteract);
	//UI::PushScale(vec2::ONE * camera_zoom * 2.0);
	forE(pencil_strokes){
		if(it->pencil_points.count > 1){
			
			//array<vec2> pps(it->pencil_points.count);
			//forI(it->pencil_points.count) pps.add(ToScreen(it->pencil_points[i]));
			//Render::DrawLines2D(pps, it->size / camera_zoom, it->color, 4, vec2::ZERO, DeshWindow->dimensions);
			
			UI::CircleFilled(ToScreen(it->pencil_points[0]), it->size/2.f, 16, it->color);
			for(int i = 1; i < it->pencil_points.count; ++i){
				UI::CircleFilled(ToScreen(it->pencil_points[i]), it->size/2.f, 16, it->color);
				UI::Line(ToScreen(it->pencil_points[i-1]), ToScreen(it->pencil_points[i]), it->size, it->color);
			}
		}
	}
	//UI::PopScale();
	UI::End();
	
	//// @draw_canvas_info ////
	UI::TextF(str8_lit("%.3f FPS"), F_AVG(50, 1 / (DeshTime->deltaTime / 1000)));
	UI::TextF(str8_lit("Active Tool:   %s"), canvas_tool_strings[active_tool]);
	UI::TextF(str8_lit("Previous Tool: %s"), canvas_tool_strings[previous_tool]);
	UI::TextF(str8_lit("Selected Element: %d"), u64(selected_element));
	UI::TextF(str8_lit("campos:  (%g, %g)"),camera_pos.x,camera_pos.y);
	UI::TextF(str8_lit("camzoom: %g"), camera_zoom);
	UI::TextF(str8_lit("camarea: (%g, %g)"), WorldViewArea().x, WorldViewArea().y);
	UI::TextF(str8_lit("graph active: %s"), (activeGraph) ? "true" : "false");
	
	UI::End();
	UI::PopVar();
}