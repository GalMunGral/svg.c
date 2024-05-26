#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "cmd.h"

typedef enum
{
  langle,
  langle_slash,
  rangle,
  slash_rangle,
  eq,
  string,
  quoted_string,
  eof,
} token_type;

typedef struct token
{
  token_type type;
  char *s;
} token;

typedef struct attr_node
{
  token name;
  token value;
  struct attr_node *next;
} attr_node;

typedef struct xml_node
{
  token tag;
  attr_node *attrs;
  struct xml_node *next;
  struct xml_node *children;
} xml_node;

typedef struct char_stream
{
  int next;
} char_stream;

int read_char(char_stream *s)
{
  int c = s->next;
  s->next = getchar();
  return c;
}

char *get_string(char_stream *s, char *end)
{
  static char buffer[4096];
  int i = 0;
  while (s->next != EOF && !(strchr(end, s->next)))
  {
    buffer[i++] = read_char(s);
  }

  if (!(strchr(end, s->next)))
    exit(1);

  buffer[i] = '\0';
  return strdup(buffer);
}

typedef struct token_stream
{
  token next;
  char_stream src;
} token_stream;

token read_token(token_stream *s)
{
  token t = s->next;
  while (isspace(s->src.next))
    read_char(&s->src);

  switch (s->src.next)
  {
  case EOF:
    s->next = (token){.type = eof};
    break;

  case '<':
    read_char(&s->src);
    if (s->src.next == '/')
    {
      read_char(&s->src);
      s->next = (token){.type = langle_slash};
    }
    else
    {
      s->next = (token){.type = langle};
    }
    break;

  case '/':
    read_char(&s->src);
    if (s->src.next != '>')
      exit(1);
    read_char(&s->src);
    s->next = (token){.type = slash_rangle};
    break;

  case '>':
    read_char(&s->src);
    s->next = (token){.type = rangle};
    break;

  case '=':
    read_char(&s->src);
    s->next = (token){.type = eq};
    break;

  case '"':
  {
    read_char(&s->src);
    char *str = get_string(&s->src, "\"");
    read_char(&s->src);
    s->next = (token){.type = quoted_string, .s = str};
    break;
  }
  default:
  {
    char *str = get_string(&s->src, " =>");
    s->next = (token){.type = string, .s = str};
    break;
  }
  }
  return t;
}

int accept(token_stream *s, token_type expected, token *dst)
{
  if (s->next.type != expected)
    return 0;
  if (dst)
    *dst = s->next;
  read_token(s);
  return 1;
}

attr_node *attr(token_stream *s)
{
  attr_node *node = calloc(1, sizeof(attr_node));
  if (!(accept(s, string, &node->name) &&
        accept(s, eq, NULL) &&
        accept(s, quoted_string, &node->value)))
    exit(1);
  return node;
}

attr_node *attr_list(token_stream *s)
{
  if (s->next.type != string)
    return NULL;

  attr_node *head = attr(s), *tail = head;
  while (s->next.type == string)
    tail = tail->next = attr(s);

  return head;
}

xml_node *xml_list(token_stream *s);

xml_node *xml(token_stream *s)
{
  xml_node *node = calloc(1, sizeof(xml_node));
  if (!(accept(s, langle, NULL) &&
        accept(s, string, &node->tag)))
    exit(1);

  node->attrs = attr_list(s);

  if (accept(s, slash_rangle, NULL))
    return node;

  if (!accept(s, rangle, NULL))
    exit(1);

  node->children = xml_list(s);

  if (!(accept(s, langle_slash, NULL) &&
        accept(s, string, NULL) &&
        accept(s, rangle, NULL)))
    exit(1);

  return node;
}

xml_node *xml_list(token_stream *s)
{
  if (s->next.type != langle)
    return NULL;

  xml_node *head = xml(s), *tail = head;
  while (s->next.type == langle)
    tail = tail->next = xml(s);

  return head;
}

typedef struct cmd_node
{
  cmd_type type;
  float x1, y1, x2, y2, x, y;
  int color;
  struct cmd_node *next;
} cmd_node;

cmd_node *head, *tail;

void emit(cmd_node *cmd)
{
  if (!head)
  {
    head = tail = cmd;
  }
  else
  {
    tail = tail->next = cmd;
  }
}

int hex(char c)
{
  if (isdigit(c))
    return c - '0';
  if (isupper(c))
    return 10 + c - 'A';
  if (islower(c))
    return 10 + c - 'a';
  return 0;
}

int parse_color(char *s)
{
  int n = strlen(s);
  if (!n || s[0] != '#')
    return 0;
  if (n == 4)
    return hex(s[1]) << 20 | hex(s[1]) << 16 | hex(s[2]) << 12 | hex(s[2]) << 8 | hex(s[3]) << 4 | hex(s[3]);
  if (n == 7)
    return hex(s[1]) << 20 | hex(s[2]) << 16 | hex(s[3]) << 12 | hex(s[4]) << 8 | hex(s[5]) << 4 | hex(s[6]);
  return 0;
}

void paint_commands(attr_node *p)
{
  for (; p; p = p->next)
  {
    if (strcmp(p->name.s, "fill") == 0)
    {
      cmd_node *cmd = calloc(1, sizeof(cmd_node));
      cmd->type = fill_color;
      cmd->color = parse_color(p->value.s);
      emit(cmd);
    }
    else if (strcmp(p->name.s, "stroke") == 0)
    {
      cmd_node *cmd = calloc(1, sizeof(cmd_node));
      cmd->type = stroke_color;
      cmd->color = parse_color(p->value.s);
      emit(cmd);
    }
    else if (strcmp(p->name.s, "stroke-width") == 0)
    {
      cmd_node *cmd = calloc(1, sizeof(cmd_node));
      cmd->type = stroke_width;
      sscanf(p->value.s, "%f", &cmd->x);
      emit(cmd);
    }
  }
}

typedef enum
{
  command,
  number,
  eos
} d_token_type;

typedef struct
{
  d_token_type type;
  union
  {
    char c;
    float f;
  } value;
} d_token;

void read_token_d(char **pptr, d_token *next_token)
{
  while (**pptr && (isspace(**pptr) || **pptr == ','))
    ++*pptr;

  if (!**pptr)
  {
    *next_token = (d_token){.type = eos};
  }
  else if (isalpha(**pptr))
  {
    char c = **pptr;
    ++*pptr;
    *next_token = (d_token){.type = command, .value.c = c};
  }
  else
  {
    float f;
    int n;
    sscanf(*pptr, "%f%n", &f, &n);
    *pptr += n;
    *next_token = (d_token){.type = number, .value.f = f};
  }
}

char accept_command(d_token *next_token, char **pptr)
{
  if (next_token->type != command)
    exit(1);
  char c = next_token->value.c;
  read_token_d(pptr, next_token);
  return c;
}

float accept_float(d_token *next_token, char **pptr)
{
  if (next_token->type != number)
    exit(1);
  float f = next_token->value.f;
  read_token_d(pptr, next_token);
  return f;
}

void stencil_commands(char *ptr)
{
  d_token next_token;
  read_token_d(&ptr, &next_token);

  cmd_type cur_type;
  cmd_node *cmd;
  while (next_token.type != eos)
  {
    char c = accept_command(&next_token, &ptr);
    switch (c)
    {
    case 'M':
    case 'm':
      cur_type = c == 'M' ? move_to : move_to_d;
      while (next_token.type == number)
      {
        cmd = calloc(1, sizeof(cmd_node));
        cmd->type = cur_type;
        cmd->x = accept_float(&next_token, &ptr);
        cmd->y = accept_float(&next_token, &ptr);
        emit(cmd);
      }
      break;

    case 'L':
    case 'l':
      cur_type = c == 'L' ? line_to : line_to_d;
      while (next_token.type == number)
      {
        cmd = calloc(1, sizeof(cmd_node));
        cmd->type = cur_type;
        cmd->x = accept_float(&next_token, &ptr);
        cmd->y = accept_float(&next_token, &ptr);
        emit(cmd);
      }
      break;

    case 'V':
    case 'v':
      cur_type = c == 'V' ? v_line_to : v_line_to_d;
      while (next_token.type == number)
      {
        cmd = calloc(1, sizeof(cmd_node));
        cmd->type = cur_type;
        cmd->y = accept_float(&next_token, &ptr);
        emit(cmd);
      }
      break;

    case 'H':
    case 'h':
      cur_type = c == 'H' ? h_line_to : h_line_to_d;
      while (next_token.type == number)
      {
        cmd = calloc(1, sizeof(cmd_node));
        cmd->type = cur_type;
        cmd->x = accept_float(&next_token, &ptr);
        emit(cmd);
      }
      break;

    case 'C':
    case 'c':
      cur_type = c == 'C' ? curve_to : curve_to_d;
      while (next_token.type == number)
      {
        cmd = calloc(1, sizeof(cmd_node));
        cmd->type = cur_type;
        cmd->x1 = accept_float(&next_token, &ptr);
        cmd->y1 = accept_float(&next_token, &ptr);
        cmd->x2 = accept_float(&next_token, &ptr);
        cmd->y2 = accept_float(&next_token, &ptr);
        cmd->x = accept_float(&next_token, &ptr);
        cmd->y = accept_float(&next_token, &ptr);
        emit(cmd);
      }
      break;

    case 'S':
    case 's':
      cur_type = c == 'S' ? s_curve_to : s_curve_to_d;
      while (next_token.type == number)
      {
        cmd = calloc(1, sizeof(cmd_node));
        cmd->type = cur_type;
        cmd->x2 = accept_float(&next_token, &ptr);
        cmd->y2 = accept_float(&next_token, &ptr);
        cmd->x = accept_float(&next_token, &ptr);
        cmd->y = accept_float(&next_token, &ptr);
        emit(cmd);
      }
      break;

    case 'z':
      cmd = calloc(1, sizeof(cmd_node));
      cmd->type = close_path;
      emit(cmd);
      break;

    default:
      exit(1);
    }
  }
  if (tail && tail->type != close_path)
  {
    cmd = calloc(1, sizeof(cmd_node));
    cmd->type = close_path;
    emit(cmd);
  }
}

void draw_commands(xml_node *node)
{
  if (strcmp(node->tag.s, "svg") == 0 || strcmp(node->tag.s, "g") == 0)
  {
    paint_commands(node->attrs);
    for (xml_node *p = node->children; p; p = p->next)
    {
      draw_commands(p);
    }
  }
  else if (strcmp(node->tag.s, "path") == 0)
  {
    paint_commands(node->attrs);
    for (attr_node *p = node->attrs; p; p = p->next)
    {
      if (strcmp(p->name.s, "d") == 0)
      {
        stencil_commands(p->value.s);
      }
    }
  }
}

int main()
{
  token_stream s;
  read_char(&s.src);
  read_token(&s);
  draw_commands(xml(&s));

  for (cmd_node *cmd = head; cmd; cmd = cmd->next)
  {
    printf("%2d%10.4f%10.4f%10.4f%10.4f%10.4f%10.4f%8x\n",
           cmd->type, cmd->x1, cmd->y1, cmd->x2, cmd->y2, cmd->x, cmd->y, cmd->color);
  }

  return 0;
}