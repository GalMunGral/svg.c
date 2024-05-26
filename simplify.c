#include <stdio.h>
#include "cmd.h"

int main()
{
  while (getchar() != EOF)
  {
    cmd_type cmd;
    int color;
    float x1, y1, x2, y2, x, y;
    scanf("%d%f%f%f%f%f%f%x", &cmd, &x1, &y1, &x2, &y2, &x, &y, &color);

    switch (cmd)
    {
    }
  }
}