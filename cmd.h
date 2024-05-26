typedef enum
{
  stroke_width,
  stroke_color,
  fill_color,
  move_to,
  move_to_d,
  line_to,
  line_to_d,
  v_line_to,
  v_line_to_d,
  h_line_to,
  h_line_to_d,
  curve_to,
  curve_to_d,
  s_curve_to,
  s_curve_to_d,
  close_path
} cmd_type;

typedef enum
{
  edge,
  fill
} s_cmd_type;