
//stolen from BYP, pretty cool!
CUSTOM_COMMAND_SIG(explorer)
CUSTOM_DOC("Opens file explorer in hot directory")
{
	Scratch_Block scratch(app);
	String_Const_u8 hot = push_hot_directory(app, scratch);
	exec_system_command(app, 0, buffer_identifier(0), hot, string_u8_litexpr("explorer ."), 0);
}

CUSTOM_COMMAND_SIG(luis_escape)
CUSTOM_DOC("escape key")
{
    //does nothing, hanlded by view_input_handler
}

CUSTOM_COMMAND_SIG(luis_toggle_modal_mode)
CUSTOM_DOC("Toggles modal mode") {	
	IN_MODAL_MODE = !IN_MODAL_MODE;
    
	View_ID view        = get_active_view(app, Access_Always);
	Buffer_ID buffer_id = view_get_buffer(app, view, Access_Always);
    update_buffer_bindings_for_modal_toggling(app, buffer_id);
}


CUSTOM_COMMAND_SIG(luis_interactive_open_or_new)  
CUSTOM_DOC("open in new in same tab") 
{	
    //my_interactive_open_or_new_internal(app, false);
	View_ID view = get_active_view(app, Access_Always);
    
    Scratch_Block scratch(app);
    
    Buffer_ID buffer_id = view_get_buffer(app, view, Access_Always);
    String_Const_u8 directory = get_directory_for_buffer(app, scratch, buffer_id);
    if(directory.size) set_hot_directory(app, directory);
    //tab_state_dont_peek_new_buffer(); 
    interactive_open_or_new(app);
    //Buffer_ID new_buffer_id = view_get_buffer(app, view, Access_Always);
    //Buffer_Tab_State *state = get_buffer_tab_state_for_view(app, view);
    //if(state && !make_new_tab) //update the current buffer id tab
    //{
    //i32 tab_index = get_tab_index_for_buffer_or_matching_cpp_buffer_id(state, new_buffer_id);
    //if(tab_index != -1) state->current_tab = tab_index;
    
    //Buffer_Tab *tab = get_current_tab(state);
    //tab->id = new_buffer_id;
    //view_set_buffer(app, view, new_buffer_id, 0); dont have to do this since call to interactive_open_or_new_same_tab does it for us
    //}
}

CUSTOM_COMMAND_SIG(luis_home)
CUSTOM_DOC("go start of visual line")
{
    View_ID view = get_active_view(app, Access_Always);
    i64 linenum = get_line_number_from_pos(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view));
    Range_i64 range = get_visual_line_start_end_pos(app, view, linenum);
    view_set_cursor_and_preferred_x(app, view, seek_pos(range.min));
}

CUSTOM_COMMAND_SIG(luis_left_word)
CUSTOM_DOC("move left")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, boundary_alpha_numeric_underscore));
}

CUSTOM_COMMAND_SIG(luis_right_word)
CUSTOM_DOC("move right")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, boundary_alpha_numeric_underscore));
}


internal void
luis_set_mark(Application_Links *app, View_ID view, i64 pos)
{
    luis_view_set_flags(app, view, VIEW_NOTEPAD_MODE_MARK_SET);
    view_set_mark(app, view, seek_pos(pos));
}

CUSTOM_COMMAND_SIG(luis_set_mark)
CUSTOM_DOC("set mark")
{
    View_ID view = get_active_view(app, Access_Always);
    luis_set_mark(app, view, view_get_cursor_pos(app, view));
}

internal b32
strmatch_so_far(String_Const_u8 a, String_Const_u8 b, i32 count)
{
    if(a.size >= count && b.size >= count)
    {
        for(i32 i = 0; i < count; i += 1)
        {
            if(a.str[i] != b.str[i])	return false;
        }
        return true;
    }
    else return false;  
}


CUSTOM_COMMAND_SIG(luis_scope_braces)
CUSTOM_DOC("writes {}")
{
    #if 1
    //write_text(app, SCu8("\n{\n\n}"));
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    Scratch_Block scratch(app);
    i64 pos = view_get_cursor_pos(app, view);
    i64 linenum = get_line_number_from_pos(app, buffer, pos);
    String_Const_u8 line = push_buffer_line(app, scratch, buffer, linenum);
    line = string_skip_whitespace(line);
    
    
    History_Group hgroup = history_group_begin(app, buffer);
    //global_history_edit_group_begin(app);
    //NOTE I check for space after the keyword to ensure it's not a substring of a bigger word
    //this will miss if there's a newline right after it. The more correct way of doing this is with tokens
    String_Const_u8 string = {}; 
    if(strmatch_so_far(SCu8("struct "), line, 7) ||
        strmatch_so_far(SCu8("enum "),   line, 5) ||
        strmatch_so_far(SCu8("union "),  line, 6))
    {
        string.str = (u8 *)"{\n\n};";
        string.size = sizeof("{\n\n};") - 1;
        move_up(app);
    }
    else if(line.size == 0)
    {
        string.str = (u8 *)"{\n\n}";
        string.size = sizeof("{\n\n}") - 1;
        move_up(app);
    }
    else
    {
        string.str = (u8 *)"{\n\n}";
        string.size = sizeof("{\n\n}") - 1;
        move_up(app);
    }
    
    buffer_replace_range(app, buffer, Ii64(pos), string);
    auto_indent_buffer(app, buffer, Ii64_size(pos, string.size));
    move_vertical_lines(app, 2);
    auto_indent_line_at_cursor(app);
    history_group_end(hgroup);
    //global_history_edit_group_end(app);
    #endif  
}

CUSTOM_COMMAND_SIG(luis_indent_range)
CUSTOM_DOC("indent_range")
{
    if(PREV_PASTE_INIT_CURSOR_POS > -1)
    {
        View_ID view = get_active_view(app, Access_ReadWriteVisible);
        Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
        Range_i64 range = Ii64(PREV_PASTE_INIT_CURSOR_POS, view_get_cursor_pos(app, view));
        if(range.min != range.max)
        {
            auto_indent_buffer(app, buffer, range);
            move_past_lead_whitespace(app, view, buffer);   
        }
    }
    else auto_indent_range(app);
}

CUSTOM_COMMAND_SIG(luis_select_line)
CUSTOM_DOC("go end of visual line")
{
    View_ID view = get_active_view(app, Access_Always);
    i64 linenum = get_line_number_from_pos(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view));
    Range_i64 range = get_visual_line_start_end_pos(app, view, linenum);
    luis_set_mark(app, view, range.min);
    view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
}

function void
luis_select_scope(Application_Links *app, View_ID view, Range_i64 range){
    view_set_cursor_and_preferred_x(app, view, seek_pos(range.first));
    luis_set_mark(app, view, range.end);
    view_look_at_region(app, view, range.first, range.end);
    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(luis_select_surrounding_scope)
CUSTOM_DOC("select surrounding scope")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Range_i64 range = {};
    if (find_surrounding_nest(app, buffer, pos, FindNest_Scope, &range)){
        luis_select_scope(app, view, range);
    }
}

CUSTOM_COMMAND_SIG(luis_select_surrounding_scope_maximal)
CUSTOM_DOC("select surrounding scope")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    Range_i64 range = {};
    if (find_surrounding_nest(app, buffer, pos, FindNest_Scope, &range)){
        for (;;){
            pos = range.min;
            if (!find_surrounding_nest(app, buffer, pos, FindNest_Scope, &range)){
                break;
            }
        }
        luis_select_scope(app, view, range);
    }
}


CUSTOM_COMMAND_SIG(luis_write_underscore)
CUSTOM_DOC("")
{	write_text(app, SCu8("_"));	}

CUSTOM_COMMAND_SIG(luis_write_pointer_arrow)
CUSTOM_DOC("")
{	write_text(app, SCu8("->"));	}

CUSTOM_COMMAND_SIG(luis_write_newline)
CUSTOM_DOC("")
{	write_text(app, SCu8("\n"));	}

CUSTOM_COMMAND_SIG(luis_write_tab)
CUSTOM_DOC("")
{	write_text(app, SCu8("\t"));	}



//this one doesn't work as well as hoped
CUSTOM_COMMAND_SIG(luis_adjust_horizontal_view_toggle)
CUSTOM_DOC("Moves view horizontally to cursor x pos or back to leftmost of screen")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    if (scroll.position.pixel_shift.x > -100 &&
        scroll.position.pixel_shift.x < 100) //on left side of screen go to cursor
    {
        i64 pos = view_get_cursor_pos(app, view);
        Buffer_Cursor cursor = view_compute_cursor(app, view, seek_pos(pos));
        Vec2_f32 p = view_relative_xy_of_pos(app, view, cursor.line, pos);
        scroll.target.pixel_shift.x = clamp_bot(0.f, p.x - 8*30.f);
    }
    else //go back to leftmost of screen
    {
        scroll.target.pixel_shift.x = 0;
    }
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
    no_mark_snap_to_cursor(app, view);
    
}


CUSTOM_COMMAND_SIG(luis_multiline_comment_toggle)
CUSTOM_DOC("Deletes all whitespace at cursor, going backwards")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 mark_pos = view_get_mark_pos(app, view);
    b32 add_comments;
    {
        Range_i64 line_range = get_visual_line_start_end_pos(app, view, get_line_number_from_pos(app, buffer_id, cursor_pos));
        add_comments = !c_line_comment_starts_at_position(app, buffer_id, line_range.min);
    }
    ///   
    Range_i64 range = {};
    range.start = Min(cursor_pos, mark_pos);
    range.end   = Max(cursor_pos, mark_pos);
    Range_i64 lines = get_line_range_from_pos_range(app, buffer_id, range);
    if(!is_valid_line_range(app, buffer_id, lines))	return;
    
    History_Group new_history_group = history_group_begin(app, buffer_id);
    for(i64 line = lines.start; line <= lines.end; line += 1)
    {
        if(!line_is_blank(app, buffer_id, line))
        {
            //i64 pos = get_line_start_pos(app, buffer_id, line);
            //i64 pos = get_visual_line_start(app, view, buffer_id, line);
            i64 pos = get_visual_line_start_end_pos(app, view, line).min;
            
            u8  test[256];
            buffer_read_range(app, buffer_id, Ii64(pos, pos + 256), test);
            if(add_comments)
            {
                if(!c_line_comment_starts_at_position(app, buffer_id, pos))
                {
                    buffer_replace_range(app, buffer_id, Ii64(pos), SCu8("//"));
                }
            }
            else
            {
                if(c_line_comment_starts_at_position(app, buffer_id, pos))
                {
                    buffer_replace_range(app, buffer_id, Ii64(pos, pos + 2), string_u8_empty);
                }
            }
        }
    }
    history_group_end(new_history_group);
}

CUSTOM_COMMAND_SIG(luis_end)
CUSTOM_DOC("go end of visual line")
{
    View_ID view = get_active_view(app, Access_Always);
    i64 linenum = get_line_number_from_pos(app, view_get_buffer(app, view, Access_Always), view_get_cursor_pos(app, view));
    Range_i64 range = get_visual_line_start_end_pos(app, view, linenum);
    view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
}

internal View_ID
luis_find_build_view(Application_Links *app)
{
    View_ID build_view = 0;
    Buffer_ID comp_buffer = get_comp_buffer(app);
    if(comp_buffer)
    {
        for(View_ID v = get_view_next(app, 0, Access_Always); v; v = get_view_next(app, v, Access_Always))
        {
            #if 0 //old tab group way
            Buffer_Tab_Group *group = view_get_tab_group(app, v);
            if(group) foreach_index_inc(i, group->tab_count)
            {
                if(group->tabs[i] == comp_buffer)
                {
                    build_view = v;
                    break;
                }
            }
            else if( view_get_buffer(app, v, Access_Always) == comp_buffer )
                build_view = v;
            #else
            if( view_get_buffer(app, v, Access_Always) == comp_buffer )
                build_view = v;
            #endif
            
            if(build_view)	break;
        }   
    }
    return build_view;
}

CUSTOM_COMMAND_SIG(luis_close_panel)
CUSTOM_DOC("Close panel. Peek first.") {
    
    View_ID active_view = get_active_view(app, Access_Always);
    if (View_ID peek = luis_get_peek_window(app, active_view)) {
        view_close(app, peek);
    }
    else  {
        for(View_ID v = get_view_next(app, 0, Access_Always); v; v = get_view_next(app, v, Access_Always)) {
            if (luis_view_has_flags(app, v, VIEW_IS_PEEK_WINDOW)) {
                view_close(app, v); 
                return;
            }
        }
        view_close(app, active_view);
    }
}

function b32
get_cpp_matching_file_dont_make(Application_Links *app, Buffer_ID buffer, Buffer_ID *buffer_out){
    b32 result = false;
    Scratch_Block scratch(app);
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer);
    if (file_name.size > 0){
        String_Const_u8 extension = string_file_extension(file_name);
        String_Const_u8 new_extensions[2] = {};
        i32 new_extensions_count = 0;
        if (string_match(extension, string_u8_litexpr("cpp")) || string_match(extension, string_u8_litexpr("cc"))){
            new_extensions[0] = string_u8_litexpr("h");
            new_extensions[1] = string_u8_litexpr("hpp");
            new_extensions_count = 2;
        }
        else if (string_match(extension, string_u8_litexpr("c"))){
            new_extensions[0] = string_u8_litexpr("h");
            new_extensions_count = 1;
        }
        else if (string_match(extension, string_u8_litexpr("h"))){
            new_extensions[0] = string_u8_litexpr("cpp");
            new_extensions[1] = string_u8_litexpr("c");
            new_extensions_count = 2;
        }
        else if (string_match(extension, string_u8_litexpr("hpp"))){
            new_extensions[0] = string_u8_litexpr("cpp");
            new_extensions_count = 1;
        }
        
        String_Const_u8 file_without_extension = string_file_without_extension(file_name);
        for (i32 i = 0; i < new_extensions_count; i += 1){
            Temp_Memory temp = begin_temp(scratch);
            String_Const_u8 new_extension = new_extensions[i];
            String_Const_u8 new_file_name = push_u8_stringf(scratch, "%.*s.%.*s", string_expand(file_without_extension), string_expand(new_extension));
            if (open_file(app, buffer_out, new_file_name, false, true)){
                result = true;
                break;
            }
            end_temp(temp);
        }
    }
    
    return(result);
}

CUSTOM_COMMAND_SIG(luis_matching_file_cpp_same_buffer)
CUSTOM_DOC("If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the other view.") {
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Buffer_ID new_buffer = 0;
    if (get_cpp_matching_file_dont_make(app, buffer, &new_buffer)){
        view_set_buffer(app, view, new_buffer, 0);
    }
}

CUSTOM_COMMAND_SIG(luis_matching_file_cpp)
CUSTOM_DOC("If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the other view.") {
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Buffer_ID new_buffer = 0;
    if (get_cpp_matching_file_dont_make(app, buffer, &new_buffer)){
        view = get_next_view_looped_primary_panels(app, view, Access_Always);
        view_set_buffer(app, view, new_buffer, 0);
        view_set_active(app, view);
    }
}

CUSTOM_COMMAND_SIG(luis_return)
CUSTOM_DOC("If the buffer in the active view is writable, inserts a character, otherwise performs goto_jump_at_cursor.")
{
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    if(buffer) //can read and write
    {
        write_text(app, SCu8("\n"));
        auto_indent_line_at_cursor(app);
    }
    else
    {
        goto_jump_at_cursor_same_panel(app);                                   
        //buffer = view_get_buffer(app, view, Access_ReadVisible);
        //if(buffer) //got buffer back as readonly
        //{
        //goto_jump_at_cursor(app);
        //lock_jump_buffer(app, buffer);
        //}
        //else leave_current_input_unhandled(app);
    }
}

CUSTOM_COMMAND_SIG(luis_build)
CUSTOM_DOC("build")
{
    //logprintf(app, "\nluis_build printing (%d groups init)...\n", BUFFER_TAB_GROUP_COUNT);
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    View_ID build_view = luis_find_build_view(app);
    if(!build_view)
        build_view = luis_get_or_split_peek_window(app, view, ViewSplit_Bottom);
    
    if(build_view)
    {
        
        
        standard_search_and_build(app, build_view, buffer);
        set_fancy_compilation_buffer_font(app);
        
        block_zero_struct(&prev_location);
        lock_jump_buffer(app, string_u8_litexpr("*compilation*"));
        
        //logprintf(app, "Built and now have %d groups open\n", BUFFER_TAB_GROUP_COUNT);
        view_set_active(app, view);
        
        
        //NOTE it's weird we have to do this but basically standard_search_and_build
        //calls some begin/end buffer calls the throws off the tab group thing
        //so we just manually set the current tab to comp buffer otherwise
        //sometimes we won't get it....
        #if 0
        Buffer_ID comp_buffer = get_comp_buffer(app);
        if(comp_buffer)
        {
            Buffer_Tab_Group *group = view_get_tab_group(app, build_view);
            if(group)
            {
                group->tabs[group->current_tab] = comp_buffer;
            }
            //view_set_buffer(app, build_view, comp_buffer, 0);
        }
        #endif
    }
}

internal void
add_nest_to_lister(Application_Links *app, Arena *temp, Lister_Block *lister, Lister_Prealloced_String status, Code_Index_Nest *nest) {
    if (nest->text.size == 0) return;
    
    /*View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    i64 linenum = get_line_number_from_pos(app, buffer, nest->open.start);
    String_Const_u8 line = push_buffer_line(app, temp, buffer, linenum); */
    
    String_u8 string = string_u8_push(temp, 512);
    
    i32  parent_count = 0;
    i32  max_parent_count = 12;
    Code_Index_Nest **parents = push_array_zero(temp, Code_Index_Nest *, max_parent_count);
    
    if (parents) for (Code_Index_Nest *parent = nest->parent; parent && parent->text.size > 0; parent = parent->parent) {
        if (parent_count < max_parent_count) {
            parents[parent_count++] = parent;
        }
        else break;
    }
    
    for (i32 i = parent_count - 1; i >= 0; i -= 1) {
        string_append(&string, parents[i]->text);
        string_append(&string, SCu8("::"));
    }
    
    Lister_Prealloced_String search_string = {};
    if (nest->flags & CODE_INDEX_NEST_IS_STRUCT) {
        search_string = lister_prealloced(push_stringf(lister->lister.current->arena, "struct %.*s%.*s", string_expand(string), string_expand(nest->text)));
    } 
    else if (nest->flags & CODE_INDEX_NEST_IS_UNION) {
        search_string = lister_prealloced(push_stringf(lister->lister.current->arena, "union %.*s%.*s", string_expand(string), string_expand(nest->text)));
    }
    else if (nest->flags & CODE_INDEX_NEST_IS_NAMESPACE) {
        search_string = lister_prealloced(push_stringf(lister->lister.current->arena, "namespace %.*s%.*s", string_expand(string), string_expand(nest->text)));
    }
    else if (nest->flags & CODE_INDEX_NEST_IS_FUNCTION) {
        search_string = lister_prealloced(push_stringf(lister->lister.current->arena, "proc %.*s%.*s", string_expand(string), string_expand(nest->text)));    
    }
    
    lister_add_item(lister->lister.current, search_string, status, (void*)nest, 0);
    
    
        //don't like how this is recursive but we shouldn't have too many levels of nests in principle
    for (i32 i = 0; i < nest->nest_array.count; i += 1) {
        Code_Index_Nest *subnest = nest->nest_array.ptrs[i];
        //NOTE(luis) I don't understand why we get a null pointer in this array sometimes... it's not a good sign honestly
        if (subnest && subnest->text.size > 0) {
            add_nest_to_lister(app, temp, lister, status, subnest);
        }
    }
}

CUSTOM_COMMAND_SIG(luis_list_nests_this_buffer) 
CUSTOM_DOC("Show code nests for this buffer") {
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, "Code Index:");
    lister_set_default_handlers(lister);
    
    code_index_lock();
    
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Code_Index_File *file = code_index_get_file(buffer);
    if(!file) return; //continue;
    
    Lister_Prealloced_String status  = lister_prealloced(push_buffer_base_name(app, lister.lister.current->arena, buffer));
    
    
    
    for (i32 nest_index = 0; nest_index < file->nest_array.count; nest_index += 1) {
        Code_Index_Nest *nest = file->nest_array.ptrs[nest_index];
        
        
        
        if (nest->text.size == 0) continue; //only nests with names can be printed
        
        Scratch_Block temp(app, scratch);
        add_nest_to_lister(app, temp, &lister, status, nest);
    }
    
    code_index_unlock();
    
    Lister_Result l_result = run_lister(app, lister);
    if (!l_result.canceled && l_result.user_data != 0) {
        Code_Index_Nest *nest = (Code_Index_Nest *)l_result.user_data;
        view_set_buffer(app, view, nest->file->buffer, 0);
        view_set_cursor_and_preferred_x(app, view, seek_pos(nest->open.first));
    }
}



CUSTOM_COMMAND_SIG(luis_list_notes_this_buffer)
CUSTOM_DOC("Show code indexes for buffer")
{
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, "Code Index:");
    lister_set_default_handlers(lister);
    
    code_index_lock();
    /*for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_Always)) */
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    {
        Code_Index_File *file = code_index_get_file(buffer);
        if(!file) return; //continue;
        
        Lister_Prealloced_String status  = lister_prealloced(push_buffer_base_name(app, lister.lister.current->arena, buffer));
        for(i32 note_index = 0; note_index < file->note_array.count; note_index += 1)
        {
            Code_Index_Note *note = file->note_array.ptrs[note_index];
            if (note->note_kind == CodeIndexNote_Function_Definition ||
                note->note_kind == CodeIndexNote_Type_Definition     ||
                note->note_kind == CodeIndexNote_Macro)
            {
                //Lister_Prealloced_String status = {};
                Lister_Prealloced_String search_string;
                if(note->note_kind == CodeIndexNote_Function)
                    search_string = lister_prealloced(push_stringf(lister.lister.current->arena, "%.*s()", string_expand(note->text)));
                else if(note->note_kind == CodeIndexNote_Macro)
                    search_string = lister_prealloced(push_stringf(lister.lister.current->arena, "# %.*s", string_expand(note->text)));
                else {
                    if (note->parent && note->parent->text.size > 0) { //big list of parent::child::child2::child3 stuff
                        Scratch_Block temp(app, scratch);
                        
                        String_u8 string = string_u8_push(temp, 512);
                        
                        i32  parent_count = 0;
                        i32  max_parent_count = 12;
                        Code_Index_Nest **parents = push_array_zero(temp, Code_Index_Nest *, max_parent_count);
                        
                        if (parents) for (Code_Index_Nest *parent = note->parent; parent && parent->text.size > 0; parent = parent->parent) {
                            if (parent_count < max_parent_count) {
                                parents[parent_count++] = parent;
                            }
                            else break;
                        }
                        
                        for (i32 i = parent_count - 1; i >= 0; i -= 1) {
                            Code_Index_Nest *nest = parents[i];
                            string_append(&string, nest->text);
                            string_append(&string, SCu8("::"));
                        }
                        string_append(&string, note->text);
                        
                        
                        search_string = lister_prealloced(push_string_copy(lister.lister.current->arena, SCu8(string)));
                        /*
                        search_string = lister_prealloced(push_stringf(lister.lister.current->arena, "%.*s::%.*s",
                                                                       string_expand(note->parent->text),
                                                                       string_expand(note->text))); */
                    } 
                    else { //fast path for top level stuff
                        search_string = lister_prealloced(push_string_copy(lister.lister.current->arena, note->text));    
                    }
                }
                
                
                lister_add_item(lister, search_string, status, (void*)note, 0);
            }
            
            //if (note->nest) {
                
            //}
        }
    }
    code_index_unlock();
    
    
    //Tiny_Jump result = {};
    Lister_Result l_result = run_lister(app, lister);
    if (!l_result.canceled && l_result.user_data != 0){
        //block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
        
        //View_ID view = get_active_view(app, Access_Always);
        //Buffer_ID init_buffer = view_get_buffer(app, view, Access_Always);
        //i64 init_pos = view_get_cursor_pos(app, view);
        
        //view_set_current_buffer_location(app, view, init_buffer, init_pos);
        Code_Index_Note *note = (Code_Index_Note *)l_result.user_data;
        view_set_buffer(app, view, note->file->buffer, 0);
        view_set_cursor_and_preferred_x(app, view, seek_pos(note->pos.first));
        //luis_center_view_top(app);
    }
    
    //if (result.buffer != 0){
        //View_ID view = get_this_ctx_view(app, Access_Always);
        //point_stack_push_view_cursor(app, view);
        //jump_to_location(app, view, result.buffer, result.pos);
    //}
}

#if 0

internal void
luis_offset_code_index(Application_Links *app, i32 offset) {
    View_ID view = get_active_view(app, Access_Always);
    //Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    //i64 cursor_pos = view_get_cursor_pos(app, view);
    
    Scratch_Block scratch(app);
    String_Const_u8 identifier = push_token_or_word_under_active_cursor(app, scratch);
    //String_Const_u8 scope_string = get_entire_scope_parents_at_pos(app, scratch, buffer, cursor_pos);
    
    Peek_Code_Index_State *state = luis_get_code_peek_state(app, view, identifier);
    if (!state) return;
    
    i32 new_index = state->index + offset;
    if (new_index < 0)	new_index = 0;
    
    i32 max_note_index = 0;
    Code_Index_Note *note = 0;
    Code_Index_Note *last_note = 0;
    for (Code_Index_Note *n = state->first_note; n; n = n->next_in_hash) {
        if (string_match(n->text, identifier)) {
            
            //String_Const_u8 note_scope_string = get_entire_scope_parents_at_pos(app, scratch, note->file->buffer, note->pos.min);
            //if (string_match(note_scope_string, scope_string)) 
            {
                if(max_note_index == new_index)
                    note = n;
                
                max_note_index += 1;
                last_note = n;    
            }
        }
    }
    
    if(new_index >= max_note_index)
        new_index = max_note_index - 1;
    
    if(!note)
        note = last_note;
    
    if(note) {
        View_ID peek = luis_get_or_split_peek_window(app, view, ViewSplit_Bottom);
        if(peek) {
            view_set_active(app, peek);
            view_set_buffer(app, peek, note->file->buffer, 0);
            view_set_cursor_and_preferred_x(app, peek, seek_pos(note->pos.first));
            view_set_mark(app, peek, seek_pos(note->pos.first));
            luis_center_view_top(app);
            
            state->index = new_index;
        }   
    }
}

CUSTOM_COMMAND_SIG(luis_code_index_prev)
CUSTOM_DOC("prev code index") {
    luis_offset_code_index(app, -1);
}

CUSTOM_COMMAND_SIG(luis_code_index_next)
CUSTOM_DOC("prev code index") {
    luis_offset_code_index(app, 1);
}
#endif



function String_Const_u8
string_remove_whitespace(String_Const_u8 string) {
    if (string.size == 0) return {};
    
    b32 currently_inside_whitespace = false;
    u64 max_whitespace_pos = 0;
    
    //only benefit of doing this in reverse order is that we do less copying potientally because we wouldn't copy around whitespace that we'd removed
    u64 index = string.size;
    while (index--) {
        b32 is_whitespace = character_is_whitespace(string.str[index]); 
        if (!currently_inside_whitespace) {
            if (is_whitespace) {
                currently_inside_whitespace = true;
                max_whitespace_pos = index + 1;
            }
        }
        else {
            if (!is_whitespace) {
                currently_inside_whitespace = false;
                u64 min_whitespace_pos = index + 1;
                if (min_whitespace_pos < max_whitespace_pos) {
                    u64 copy_length = string.size - max_whitespace_pos;
                    if (copy_length > 0) {
                        block_copy(string.str + min_whitespace_pos, string.str + max_whitespace_pos, copy_length);
                    }
                    string.size -= (max_whitespace_pos - min_whitespace_pos);
                }
            }
        }
    }
    
    if (currently_inside_whitespace) {
        u64 min_whitespace_pos = 0;
        if (min_whitespace_pos < max_whitespace_pos) {
            u64 copy_length = string.size - max_whitespace_pos;
            if (copy_length > 0) {
                block_copy(string.str + min_whitespace_pos, string.str + max_whitespace_pos, copy_length);
            }
            string.size -= (max_whitespace_pos - min_whitespace_pos);
        }
    }
    
    return string;
}

function Peek_Code_Index_State *
get_code_peek_state(Application_Links *app, View_ID view, i64 pos, View_ID *peek) {
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    *peek = luis_get_or_split_peek_window(app, view, ViewSplit_Bottom);
    
    Managed_Scope peek_scope = view_get_managed_scope(app, *peek);
    Peek_Code_Index_State *state = scope_attachment(app, peek_scope, view_code_peek_state, Peek_Code_Index_State);
    if (!state) {
        view_close(app, *peek);
        *peek = 0;
        return 0;   
    }
    
    
    Scratch_Block scratch(app);
    
    Scope_Prefix manual_scope_prefix = {};
    String_Const_u8 identifier = {};
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens) {
        Token *identifier_token = get_token_from_pos(app, &token_array, pos);
        //size check is probably redundant here
        if (identifier_token && identifier_token->kind == TokenBaseKind_Identifier) {
            Range_i64 range = Ii64(identifier_token);
            identifier = push_buffer_range(app, scratch, buffer, range);
            
            Token *first_identifier = 0;
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, identifier_token->pos - 1);
            Token *at = token_it_read(&it);
            
            auto is_scope_operator = [](Token *token) -> bool {
                if (token && token->kind == TokenBaseKind_Operator && token->sub_kind == TokenCppKind_ColonColon) {
                    return true;
                }    
                else return false;
            };
            
            if (is_scope_operator(at)) {
                b32 began_with_scope_operator = true;
                manual_scope_prefix.scope_count = 1;
                token_it_dec_non_whitespace(&it);
                at = token_it_read(&it);
                
                while (at) {
                    if (at->kind == TokenBaseKind_Identifier) {
                        first_identifier = at;
                        began_with_scope_operator = false;
                    }
                    else if (is_scope_operator(at)) {
                        manual_scope_prefix.scope_count += 1;
                        began_with_scope_operator = true;
                        /*keep scanning*/
                    }
                    else break;
                    
                    token_it_dec_non_whitespace(&it);
                    at = token_it_read(&it);
                }
                
                if (first_identifier) {
                    range.min = first_identifier->pos;
                    range.max = identifier_token->pos;
                    manual_scope_prefix.string = push_buffer_range(app, scratch, buffer, range);;
                    manual_scope_prefix.string = string_remove_whitespace(manual_scope_prefix.string);
                    if (!began_with_scope_operator) {
                        manual_scope_prefix.string = push_stringf(scratch, "::%.*s", string_expand(manual_scope_prefix.string));
                        manual_scope_prefix.scope_count += 1;
                    }
                }
            }
            
            
        }
    }
    else {
        view_close(app, *peek);
        *peek = 0;
        return 0;
    }
    
    
    //String_Const_u8 identifier = push_token_or_word_under_pos(app, scratch, buffer, pos);
    if (identifier.size == 0) {
        view_close(app, *peek);
        *peek = 0;
        return 0;
    }
    
    state->is_first_time_getting = false;
    
    if (state->initialized) { //check if user moved onto another word. If so just mark uninitialized and check next if statement
        if (!string_match(identifier, SCu8(state->identifier))) {
            state->initialized = false;
            
            state->index = 0;
            state->note_count = 0;
        }
    }
    
    if (!state->initialized) {
        state->initialized = true;
        state->is_first_time_getting = true;
        
        state->identifier.str  = state->identifier_buffer;
        state->identifier.size = 0;
        state->identifier.cap  = ArrayCount(state->identifier_buffer);
        string_append(&state->identifier, identifier);
        
        Scope_Prefix scope_prefix = manual_scope_prefix;
        if (scope_prefix.scope_count == 0) {
            scope_prefix = get_entire_scope_prefix(app, scratch, buffer, pos);
        }
        
        
        auto get_match_metric = [&scope_prefix] (Scope_Prefix prefix) -> i32{
            i32 search_length = (i32)Min(prefix.string.size, scope_prefix.string.size);
            i32 match_count = 0;
            for (i32 i = 0; i < search_length; i += 1) {
                if (prefix.string.str[i] == scope_prefix.string.str[i]) {
                    if (i > 0 && prefix.string.str[i-1] == ':' && prefix.string.str[i] == ':') {
                        match_count += 1;
                    }
                }
                else break;
            }
            
            i32 non_match_count = prefix.scope_count - match_count;
            //i32 non_match_count = scope_prefix_scope_count - match_count;
            return match_count - 2*non_match_count;
        };
        
        Code_Index_Note *first_note = code_index_note_from_string(identifier);
        for (Code_Index_Note *note = first_note; note; note = note->next_in_hash) {
            if (state->note_count == countof(state->notes)) break;
            
            //ignore type and function decls
            if (note->note_kind == CodeIndexNote_Type_Definition     ||
                note->note_kind == CodeIndexNote_Function_Definition ||
                note->note_kind == CodeIndexNote_Macro               ||
                note->note_kind == CodeIndexNote_Namespace) 
            {    
                if (string_match(note->text, identifier)) 
                {
                    //the higher the number, the better the result will be
                    
                    
                    state->notes[state->note_count++] = note;
                    Scope_Prefix note_prefix = get_entire_scope_prefix(app, scratch, note->file->buffer, note->pos.min);
                    i32 note_metric = get_match_metric(note_prefix);
                    
                	//add to array already sorted
                    for (i32 i = state->note_count - 2; i >= 0; i -= 1) {
                        Code_Index_Note *prev = state->notes[i];
                        Assert (state->notes[i+1] == note); //NOTE (i+1) should always be our note
                        
                        Scope_Prefix prev_prefix = get_entire_scope_prefix(app, scratch, prev->file->buffer, prev->pos.min);
                        i32 prev_metric = get_match_metric(prev_prefix);
                        
                        if (prev_metric < note_metric) {
                            SWAP(state->notes[i], state->notes[i+1]); 
                        }
                        else break; //we are ordered nothing else to do
                    }
                }    
            }
            
        }
        
        
        
        #if 0
        int k = 0;
        
        for (i32 i = 0; i < state->note_count; i += 1) {
            Code_Index_Note *note = state->notes[i];
            Buffer_ID note_buffer = note->file->buffer;
            
            String_Const_u8 note_prefix = get_entire_scope_parents_at_pos(app, scratch, note->file->buffer, note->pos.min);
            print_message(app, note_prefix);
            print_message(app, note->text);
            print_message(app, SCu8(" @ "));
            
            String_Const_u8 filepath = push_buffer_file_name(app, scratch, note_buffer);
            print_message(app, filepath);
            print_message(app, SCu8("\n\n"));
            
            k += 1;
        }
        
         
        k -= 1;
        #endif
        
        if (state->note_count == 0) {
            view_close(app, *peek);
            *peek = 0;
            state = 0;
        }
    }
    return state;
}


CUSTOM_COMMAND_SIG(luis_code_index_prev)
CUSTOM_DOC("prev code index") {
    View_ID view = get_active_view(app, Access_Always);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    View_ID peek;
    Peek_Code_Index_State *state = get_code_peek_state(app, view, cursor_pos, &peek);
    if (state && state->note_count > 0) {
        if (!state->is_first_time_getting) {
            state->index = Max(state->index - 1, 0);    
        }
        Code_Index_Note *note = state->notes[state->index];
        
        
        view_set_active(app, peek);
        view_set_buffer(app, peek, note->file->buffer, 0);
        view_set_cursor_and_preferred_x(app, peek, seek_pos(note->pos.first));
        view_set_mark(app, peek, seek_pos(note->pos.first));
        luis_center_view_top(app);
    }
}

CUSTOM_COMMAND_SIG(luis_code_index_next)
CUSTOM_DOC("prev code index") {
    View_ID view = get_active_view(app, Access_Always);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    View_ID peek;
    Peek_Code_Index_State *state = get_code_peek_state(app, view, cursor_pos, &peek);
    if (state && state->note_count > 0) {
        
        if (!state->is_first_time_getting) {
            state->index = Min(state->index + 1, state->note_count - 1);    
        }
        Code_Index_Note *note = state->notes[state->index];
        
        
        view_set_active(app, peek);
        view_set_buffer(app, peek, note->file->buffer, 0);
        view_set_cursor_and_preferred_x(app, peek, seek_pos(note->pos.first));
        view_set_mark(app, peek, seek_pos(note->pos.first));
        luis_center_view_top(app);
    }
} 


internal void
luis_isearch(Application_Links *app, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 buffer_size = buffer_get_size(app, buffer);
    if(buffer_size == 0)	return;
    
    
    
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    if(!start_query_bar(app, &bar, 0))	return;
    
    Vec2_f32 old_margin = {};
    Vec2_f32 old_push_in = {};
    view_get_camera_bounds(app, view, &old_margin, &old_push_in);
    
    Vec2_f32 margin = old_margin;
    margin.y = clamp_bot(200.f, margin.y);
    view_set_camera_bounds(app, view, margin, old_push_in);
    
    Scan_Direction scan = start_scan;
    i64 pos = first_pos;
    
    u8 bar_string_space[256];
    bar.string = SCu8(bar_string_space, query_init.size);
    block_copy(bar.string.str, query_init.str, query_init.size);
    u64 match_size = bar.string.size;
    
    #define BAR_APPEND_STRING(string__) \
    do \
    { \
        String_Const_u8 string = (string__); \
        String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space)); \
        string_append(&bar_string, string); \
        bar.string = bar_string.string; \
        string_change = true; \
    } while(0)
    
    b32 save_search_string = true;
    b32 move_to_new_pos = false;
    User_Input in = {};
    for (;;)
    {
        bar.prompt = (scan == Scan_Forward) ? string_u8_litexpr("I-Search: ") : string_u8_litexpr("Reverse-I-Search: ");
        isearch__update_highlight(app, view, Ii64_size(pos, match_size));
        
        in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (in.abort)	break;
        
        b32 string_change = false;
        b32 do_scan_action = false;
        b32 do_scroll_wheel = false;
        Scan_Direction change_scan = scan;
        if(in.event.kind == InputEventKind_KeyStroke)
        {
            Key_Code code = in.event.key.code;
            b32 ctrl_down = has_modifier(&in.event.key.modifiers, KeyCode_Control);
            
            if(code == KeyCode_Return || code == KeyCode_Tab)
            {
                if(ctrl_down) //append previous search string
                {
                    bar.string.size = cstring_length(previous_isearch_query);
                    block_copy(bar.string.str, previous_isearch_query, bar.string.size);
                }
                else
                {
                    //u64 size = bar.string.size;
                    //size = clamp_top(size, sizeof(previous_isearch_query) - 1);
                    //block_copy(previous_isearch_query, bar.string.str, size);
                    //previous_isearch_query[size] = 0;
                    move_to_new_pos = true;
                    break;
                }
            }
            else if(code == KeyCode_Backspace)
            {
                if(ctrl_down)
                {
                    if (bar.string.size > 0){
                        string_change = true;
                        bar.string.size = 0;
                    }
                }
                else
                {
                    u64 old_bar_string_size = bar.string.size;
                    bar.string = backspace_utf8(bar.string);
                    string_change = (bar.string.size < old_bar_string_size);
                }
            }
            else
            {
                View_Context ctx = view_current_context(app, view);
                Mapping *mapping = ctx.mapping;
                Command_Map *map = mapping_get_map(mapping, ctx.map_id);
                Command_Binding binding = map_get_binding_recursive(mapping, map, &in.event);
                if (binding.custom != 0)
                {
                    if(binding.custom == luis_fsearch || binding.custom == luis_rsearch)
                    {
                        if(binding.custom == luis_fsearch)
                        {
                            change_scan = Scan_Forward;
                            do_scan_action = true;
                            if(bar.string.size == 0)
                            {
                                bar.string.size = cstring_length(previous_isearch_query);
                                block_copy(bar.string.str, previous_isearch_query, bar.string.size);
                            }
                        }
                        else if(binding.custom == luis_rsearch)
                        {
                            change_scan = Scan_Backward;
                            do_scan_action = true;
                            if(bar.string.size == 0)
                            {
                                bar.string.size = cstring_length(previous_isearch_query);
                                block_copy(bar.string.str, previous_isearch_query, bar.string.size);
                            }
                        }
                    }
                    else if (binding.custom == luis_write_underscore)
                        BAR_APPEND_STRING(SCu8("_"));
                    else if (binding.custom == luis_write_pointer_arrow)
                        BAR_APPEND_STRING(SCu8("->"));
                    else if(binding.custom == word_complete)
                    {
                        if(bar.string.size == 0)
                        {
                            change_scan = Scan_Forward;
                            do_scan_action = true;
                            
                            Token *token = get_token_from_pos(app, buffer, pos);
                            if(token && token->size > 0 && token->kind == TokenBaseKind_Identifier)
                            {
                                Scratch_Block scratch(app);
                                String_Const_u8 word = push_buffer_range(app, scratch, buffer, Ii64(token));
                                bar.string.size = word.size;
                                block_copy(bar.string.str, word.str, bar.string.size);
                                
                                pos = token->pos - 1;
                                if(pos < 0) pos = 0;
                            }
                        }
                    }
                    else
                    {
                        Command_Metadata *metadata = get_command_metadata(binding.custom);
                        if (metadata != 0){
                            if (metadata->is_ui){
                                view_enqueue_command_function(app, view, binding.custom);
                                break;
                            }
                        }
                        binding.custom(app);
                    }
                }
                else	leave_current_input_unhandled(app);
            }
            
        }
        else if(in.event.kind == InputEventKind_TextInsert)
            BAR_APPEND_STRING(to_writable(&in));
        
        if (string_change){
            switch (scan){
            case Scan_Forward:
            {
                i64 new_pos = 0;
                seek_string_insensitive_forward(app, buffer, pos - 1, 0, bar.string, &new_pos);
                if (new_pos < buffer_size){
                    pos = new_pos;
                    match_size = bar.string.size;
                }
            }break;
            
            case Scan_Backward:
            {
                i64 new_pos = 0;
                seek_string_insensitive_backward(app, buffer, pos + 1, 0, bar.string, &new_pos);
                if (new_pos >= 0){
                    pos = new_pos;
                    match_size = bar.string.size;
                }
            }break;
            }
        }
        else if (do_scan_action){
            scan = change_scan;
            switch (scan){
            case Scan_Forward:
            {
                i64 new_pos = 0;
                seek_string_insensitive_forward(app, buffer, pos, 0, bar.string, &new_pos);
                if (new_pos < buffer_size){
                    pos = new_pos;
                    match_size = bar.string.size;
                }
            }break;
            
            case Scan_Backward:
            {
                i64 new_pos = 0;
                seek_string_insensitive_backward(app, buffer, pos, 0, bar.string, &new_pos);
                if (new_pos >= 0){
                    pos = new_pos;
                    match_size = bar.string.size;
                }
            }break;
            }
        }
        else if (do_scroll_wheel){
            mouse_wheel_scroll(app);
        }
    }
    
    view_disable_highlight_range(app, view);
    
    if (move_to_new_pos)
    {
        view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
        View_Buffer_Location *loc = view_get_prev_buffer_location(app, view);
        if(loc)
        {
            loc->buffer = buffer;
            loc->cursor = first_pos;
        }
    }
    else 
        view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
    
    if(save_search_string)
    {
        u64 size = bar.string.size;
        size = clamp_top(size, sizeof(previous_isearch_query) - 1);
        block_copy(previous_isearch_query, bar.string.str, size);
        previous_isearch_query[size] = 0;
    }
    
    view_set_camera_bounds(app, view, old_margin, old_push_in);
}

CUSTOM_COMMAND_SIG(luis_fsearch)
CUSTOM_DOC("search forwards")
{
    View_ID view = get_active_view(app, Access_Always);
    luis_isearch(app, Scan_Forward, view_get_cursor_pos(app, view), SCu8());
}

CUSTOM_COMMAND_SIG(luis_rsearch)
CUSTOM_DOC("search backwards")
{
    View_ID view = get_active_view(app, Access_Always);
    luis_isearch(app, Scan_Backward, view_get_cursor_pos(app, view), SCu8());
}