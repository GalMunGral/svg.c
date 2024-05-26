#include <stdio.h>
#include "cmd.h"

const char *format = "%2d%10.4f%10.4f%10.4f%10.4f%8x\n";

int main()
{
  cmd_type prev_cmd;
  float prev_x2, prev_y2, cur_x, cur_y;
  float cur_stroke_width;
  int cur_stroke_color;
  int cur_fill_color;

  while (getchar() != EOF)
  {
    cmd_type cmd;
    int color;
    float x1, y1, x2, y2, x, y;
    scanf("%d%f%f%f%f%f%f%x", &cmd, &x1, &y1, &x2, &y2, &x, &y, &color);

    switch (cmd)
    {
    case stroke_width:
      cur_stroke_width = x;
      break;
    case stroke_color:
      cur_stroke_color = color;
      break;
    case fill_color:
      cur_fill_color = color;
      break;
    case move_to:
      cur_x = x;
      cur_y = y;
      printf(format, edge);
      break;
    case move_to_d:
      cur_x += x;
      cur_y += y;
      printf(format, move_to, x1, y1, x2, y2, cur_x, cur_x, color);
      break;
    case line_to:
    case line_to_d:
    case v_line_to:
    case v_line_to_d:
    case h_line_to:
    case h_line_to_d:
    case curve_to:
    case curve_to_d:
    case s_curve_to:
    case s_curve_to_d:
    case close_path:
      break;
    }

    prev_cmd = cmd;
  }
  return 0;
}