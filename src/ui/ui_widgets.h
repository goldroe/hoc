#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

internal void ui_row();
internal void ui_row_end();
internal UI_Signal ui_label(String8 name);
internal UI_Signal ui_labelf(const char *fmt, ...);
internal UI_Signal ui_button(String8 name);
internal UI_Signal ui_buttonf(const char *fmt, ...);
internal UI_Signal ui_slider(String8 name, f32 *value, f32 min, f32 max);
internal UI_Signal ui_line_edit(String8 name, String8 *edit_string);
internal UI_Signal ui_directory_list(String8 directory);

#endif // UI_WIDGETS_H
