global Hoc_Command hoc_commands[] = {
{ str8_lit("nil_command"), nil_command },
{ str8_lit("self_insert"), self_insert },
{ str8_lit("quit_hoc"), quit_hoc },
{ str8_lit("newline"), newline },
{ str8_lit("backward_char"), backward_char },
{ str8_lit("forward_char"), forward_char },
{ str8_lit("forward_word"), forward_word },
{ str8_lit("backward_word"), backward_word },
{ str8_lit("backward_paragraph"), backward_paragraph },
{ str8_lit("forward_paragraph"), forward_paragraph },
{ str8_lit("previous_line"), previous_line },
{ str8_lit("next_line"), next_line },
{ str8_lit("backward_delete_char"), backward_delete_char },
{ str8_lit("forward_delete_char"), forward_delete_char },
{ str8_lit("backward_delete_word"), backward_delete_word },
{ str8_lit("forward_delete_word"), forward_delete_word },
{ str8_lit("kill_line"), kill_line },
{ str8_lit("open_line"), open_line },
{ str8_lit("save_buffer"), save_buffer },
{ str8_lit("set_mark"), set_mark },
{ str8_lit("goto_beginning_of_line"), goto_beginning_of_line },
{ str8_lit("goto_end_of_line"), goto_end_of_line },
{ str8_lit("goto_first_line"), goto_first_line },
{ str8_lit("goto_last_line"), goto_last_line },
{ str8_lit("find_file"), find_file },
};