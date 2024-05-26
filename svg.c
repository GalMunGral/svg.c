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

int next_char;

void read_char(void)
{
  next_char = getchar();
}

char *get_string(char *end)
{
  static char buffer[4096];
  int i = 0;
  while (next_char != EOF && !(strchr(end, next_char)))
  {
    buffer[i++] = next_char;
    read_char();
  }

  if (!(strchr(end, next_char)))
    exit(1);

  buffer[i] = '\0';
  return strdup(buffer);
}

token next_token;

void read_token()
{
  while (isspace(next_char))
    read_char();

  switch (next_char)
  {
  case EOF:
    next_token = (token){.type = eof};
    break;

  case '<':
    read_char();
    if (next_char == '/')
    {
      read_char();
      next_token = (token){.type = langle_slash};
    }
    else
    {
      next_token = (token){.type = langle};
    }
    break;

  case '/':
    read_char();
    if (next_char != '>')
      exit(1);
    read_char();
    next_token = (token){.type = slash_rangle};
    break;

  case '>':
    read_char();
    next_token = (token){.type = rangle};
    break;

  case '=':
    read_char();
    next_token = (token){.type = eq};
    break;

  case '"':
  {
    read_char();
    char *s = get_string("\"");
    read_char();
    next_token = (token){.type = quoted_string, .s = s};
    break;
  }
  default:
  {
    char *s = get_string(" =>");
    next_token = (token){.type = string, .s = s};
    break;
  }
  }
}

int accept(token_type expected, token *dst)
{
  if (next_token.type != expected)
    return 0;
  if (dst)
    *dst = next_token;
  read_token();
  return 1;
}

attr_node *attr(void)
{
  attr_node *node = calloc(1, sizeof(attr_node));
  if (!accept(string, &node->name))
    exit(1);
  if (!accept(eq, NULL))
    exit(1);
  if (!accept(quoted_string, &node->value))
    exit(1);
  return node;
}

attr_node *attr_list(void)
{
  if (next_token.type != string)
    return NULL;
  attr_node *head = attr(), *tail = head;
  while (next_token.type == string)
    tail = tail->next = attr();
  return head;
}

xml_node *xml(void)
{
  xml_node *node = calloc(1, sizeof(xml_node));
  if (!accept(langle, NULL))
    exit(1);

  if (!accept(string, &node->tag))
    exit(1);

  node->attrs = attr_list();

  if (accept(slash_rangle, NULL))
    return node;

  if (!accept(rangle, NULL))
    exit(1);

  if (next_token.type == langle)
  {
    xml_node *head = xml(), *tail = head;
    while (next_token.type == langle)
    {
      tail = tail->next = xml();
    }
    node->children = head;
  }

  if (!accept(langle_slash, NULL))
    exit(1);
  if (!accept(string, NULL))
    exit(1);
  if (!accept(rangle, NULL))
    exit(1);

  return node;
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
      cmd->type = stroke_color;
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

  while (next_token.type != eos)
  {
    cmd_type cur_type;
    cmd_node *cmd;

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
}

void gen_commmands(xml_node *node)
{
  if (strcmp(node->tag.s, "svg") == 0 || strcmp(node->tag.s, "g") == 0)
  {
    paint_commands(node->attrs);
    for (xml_node *p = node->children; p; p = p->next)
    {
      gen_commmands(p);
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
  read_char();
  read_token();
  gen_commmands(xml());

  for (cmd_node *cmd = head; cmd; cmd = cmd->next)
  {
    printf("%2d\t%10.4f\t%10.4f\t%10.4f\t%10.4f\t%10.4f\t%10.4f\t%6x\n",
           cmd->type, cmd->x1, cmd->y1, cmd->x2, cmd->y2, cmd->x, cmd->y, cmd->color);
  }

  return 0;
}