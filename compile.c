#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"

typedef struct char_stream {
  int next;
} char_stream;

int read_char(char_stream *s) {
  int c = s->next;
  s->next = getchar();
  return c;
}

char *get_string(char_stream *s, char *end) {
  static char buffer[4096];
  int i = 0;
  while (s->next != EOF && !(strchr(end, s->next))) {
    buffer[i++] = read_char(s);
  }

  if (!(strchr(end, s->next))) exit(1);

  buffer[i] = '\0';
  return strdup(buffer);
}

typedef enum {
  langle,
  langle_slash,
  rangle,
  slash_rangle,
  eq,
  string,
  quoted_string,
  eof,
} token_type;

typedef struct token {
  token_type type;
  char *str;
} token;
typedef struct token_stream {
  token next;
  char_stream src;
} token_stream;

token read_token(token_stream *s) {
  token t = s->next;
  while (isspace(s->src.next)) read_char(&s->src);

  switch (s->src.next) {
    case EOF:
      s->next = (token){.type = eof};
      break;

    case '<':
      read_char(&s->src);
      if (s->src.next == '/') {
        read_char(&s->src);
        s->next = (token){.type = langle_slash};
      } else {
        s->next = (token){.type = langle};
      }
      break;

    case '/':
      read_char(&s->src);
      if (s->src.next != '>') exit(1);
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

    case '"': {
      read_char(&s->src);
      char *str = get_string(&s->src, "\"");
      read_char(&s->src);
      s->next = (token){.type = quoted_string, .str = str};
      break;
    }
    default: {
      char *str = get_string(&s->src, " =>");
      s->next = (token){.type = string, .str = str};
      break;
    }
  }
  return t;
}

int accept(token_stream *s, token_type expected, token *dst) {
  if (s->next.type != expected) return 0;
  if (dst) *dst = s->next;
  read_token(s);
  return 1;
}

typedef struct attr_node {
  token name;
  token value;
  struct attr_node *next;
} attr_node;

typedef struct xml_node {
  token tag;
  attr_node *attrs;
  struct xml_node *next;
  struct xml_node *children;
} xml_node;

attr_node *attr(token_stream *s) {
  attr_node *node = calloc(1, sizeof(attr_node));
  if (!(accept(s, string, &node->name) && accept(s, eq, NULL) &&
        accept(s, quoted_string, &node->value)))
    exit(1);
  return node;
}

attr_node *attr_list(token_stream *s) {
  if (s->next.type != string) return NULL;

  attr_node *head = attr(s), *tail = head;
  while (s->next.type == string) tail = tail->next = attr(s);

  return head;
}

xml_node *xml_list(token_stream *s);

xml_node *xml(token_stream *s) {
  xml_node *node = calloc(1, sizeof(xml_node));
  if (!(accept(s, langle, NULL) && accept(s, string, &node->tag))) exit(1);

  node->attrs = attr_list(s);

  if (accept(s, slash_rangle, NULL)) return node;

  if (!accept(s, rangle, NULL)) exit(1);

  node->children = xml_list(s);

  if (!(accept(s, langle_slash, NULL) && accept(s, string, NULL) &&
        accept(s, rangle, NULL)))
    exit(1);

  return node;
}

xml_node *xml_list(token_stream *s) {
  if (s->next.type != langle) return NULL;

  xml_node *head = xml(s), *tail = head;
  while (s->next.type == langle) tail = tail->next = xml(s);

  return head;
}

typedef struct cmd_node {
  cmd_type type;
  union {
    int fill_color;
    int stroke_color;
    float stroke_width;
    struct {
      float a, b, c, d, e, f;
    } matrix;
    struct {
      float x1, y1, x2, y2, x, y;
    } path;
  } args;
  struct cmd_node *next;
} cmd_node;

typedef struct cmd_list {
  cmd_node *head;
  cmd_node *tail;
} cmd_list;

void append(cmd_list *l, cmd_node *cmd) {
  if (!l->head)
    l->head = l->tail = cmd;
  else
    l->tail = l->tail->next = cmd;
}

int hex(char c) {
  if (isdigit(c)) return c - '0';
  if (isupper(c)) return 10 + c - 'A';
  if (islower(c)) return 10 + c - 'a';
  return 0;
}

int parse_color(char *s) {
  int n = strlen(s);
  if (n == 0 || s[0] != '#') return -1;
  if (n == 4)
    return hex(s[1]) << 20 | hex(s[1]) << 16 | hex(s[2]) << 12 |
           hex(s[2]) << 8 | hex(s[3]) << 4 | hex(s[3]);
  if (n == 7)
    return hex(s[1]) << 20 | hex(s[2]) << 16 | hex(s[3]) << 12 |
           hex(s[4]) << 8 | hex(s[5]) << 4 | hex(s[6]);
  return 0;
}

typedef struct transform_token {
  enum {
    t_lparen,
    t_rparen,
    t_func,
    t_arg,
    t_eos,
  } type;
  union {
    char *s;
    float f;
  } value;
} transform_token;

typedef struct transform_token_stream {
  transform_token next;
  char *src;
} transform_token_stream;

transform_token read_transform_token(transform_token_stream *s) {
  transform_token t = s->next;

  while (isspace(*s->src) || *s->src == ',') ++s->src;

  if (!*s->src) {
    s->next = (transform_token){.type = t_eos};
  } else if (*s->src == '(') {
    s->next = (transform_token){.type = t_lparen};
    ++s->src;
  } else if (*s->src == ')') {
    s->next = (transform_token){.type = t_rparen};
    ++s->src;
  } else if (isalpha(*s->src)) {
    int n;
    char buffer[32];
    if (sscanf(s->src, "%[a-z]%n", buffer, &n) == 0) exit(1);
    s->next = (transform_token){.type = t_func, .value.s = strdup(buffer)};
    s->src += n;
  } else {
    int n;
    float f;
    if (sscanf(s->src, "%f%n", &f, &n) == 0) exit(1);
    s->next = (transform_token){.type = t_arg, .value.f = f};
    s->src += n;
  }
  return t;
}

char *transform_func(transform_token_stream *s) {
  transform_token t = read_transform_token(s);
  if (t.type != t_func) exit(1);
  return t.value.s;
}

float transform_arg(transform_token_stream *s) {
  transform_token t = read_transform_token(s);
  if (t.type != t_arg) exit(1);
  return t.value.f;
}

void compile_matrix(cmd_list *l, char *transform) {
  transform_token_stream s = {.src = transform};
  read_transform_token(&s);

  char *func_name = transform_func(&s);
  if (strcmp(func_name, "matrix") == 0) {
    cmd_node *cmd = calloc(1, sizeof(cmd_node));
    cmd->type = push_matrix;
    if (read_transform_token(&s).type != t_lparen) exit(1);
    cmd->args.matrix.a = transform_arg(&s);
    cmd->args.matrix.b = transform_arg(&s);
    cmd->args.matrix.c = transform_arg(&s);
    cmd->args.matrix.d = transform_arg(&s);
    cmd->args.matrix.e = transform_arg(&s);
    cmd->args.matrix.f = transform_arg(&s);
    if (read_transform_token(&s).type != t_rparen) exit(1);
    append(l, cmd);
  }
  free(func_name);
}

typedef struct path_token {
  enum { p_command, p_coord, p_eos } type;
  union {
    char c;
    float f;
  } value;
} path_token;

typedef struct path_token_stream {
  path_token next;
  char *src;
} path_token_stream;

path_token read_path_token(path_token_stream *s) {
  path_token t = s->next;

  while (isspace(*s->src) || *s->src == ',') ++s->src;

  if (!*s->src) {
    s->next = (path_token){.type = p_eos};
  } else if (isalpha(*s->src)) {
    s->next = (path_token){.type = p_command, .value.c = *s->src};
    ++s->src;
  } else {
    int n;
    float f;
    if (sscanf(s->src, "%f%n", &f, &n) == 0) exit(1);
    s->next = (path_token){.type = p_coord, .value.f = f};
    s->src += n;
  }
  return t;
}

char path_command(path_token_stream *s) {
  path_token t = read_path_token(s);
  if (t.type != p_command) exit(1);
  return t.value.c;
}

float path_coord(path_token_stream *s) {
  path_token t = read_path_token(s);
  if (t.type != p_coord) exit(1);
  return t.value.f;
}

void compile_path(cmd_list *l, char *d) {
  path_token_stream s = {.src = d};
  read_path_token(&s);

  cmd_type cur_type;
  cmd_node *cmd;

  cmd = calloc(1, sizeof(cmd_node));
  cmd->type = begin_path;
  append(l, cmd);

  while (s.next.type != p_eos) {
    char c = path_command(&s);
    switch (c) {
      case 'M':
      case 'm':
        cur_type = c == 'M' ? move_to : move_to_d;
        while (s.next.type == p_coord) {
          cmd = calloc(1, sizeof(cmd_node));
          cmd->type = cur_type;
          cmd->args.path.x = path_coord(&s);
          cmd->args.path.y = path_coord(&s);
          append(l, cmd);
        }
        break;

      case 'L':
      case 'l':
        cur_type = c == 'L' ? line_to : line_to_d;
        while (s.next.type == p_coord) {
          cmd = calloc(1, sizeof(cmd_node));
          cmd->type = cur_type;
          cmd->args.path.x = path_coord(&s);
          cmd->args.path.y = path_coord(&s);
          append(l, cmd);
        }
        break;

      case 'V':
      case 'v':
        cur_type = c == 'V' ? v_line_to : v_line_to_d;
        while (s.next.type == p_coord) {
          cmd = calloc(1, sizeof(cmd_node));
          cmd->type = cur_type;
          cmd->args.path.y = path_coord(&s);
          append(l, cmd);
        }
        break;

      case 'H':
      case 'h':
        cur_type = c == 'H' ? h_line_to : h_line_to_d;
        while (s.next.type == p_coord) {
          cmd = calloc(1, sizeof(cmd_node));
          cmd->type = cur_type;
          cmd->args.path.x = path_coord(&s);
          append(l, cmd);
        }
        break;

      case 'C':
      case 'c':
        cur_type = c == 'C' ? curve_to : curve_to_d;
        while (s.next.type == p_coord) {
          cmd = calloc(1, sizeof(cmd_node));
          cmd->type = cur_type;
          cmd->args.path.x1 = path_coord(&s);
          cmd->args.path.y1 = path_coord(&s);
          cmd->args.path.x2 = path_coord(&s);
          cmd->args.path.y2 = path_coord(&s);
          cmd->args.path.x = path_coord(&s);
          cmd->args.path.y = path_coord(&s);
          append(l, cmd);
        }
        break;

      case 'S':
      case 's':
        cur_type = c == 'S' ? s_curve_to : s_curve_to_d;
        while (s.next.type == p_coord) {
          cmd = calloc(1, sizeof(cmd_node));
          cmd->type = cur_type;
          cmd->args.path.x2 = path_coord(&s);
          cmd->args.path.y2 = path_coord(&s);
          cmd->args.path.x = path_coord(&s);
          cmd->args.path.y = path_coord(&s);
          append(l, cmd);
        }
        break;

      case 'z':
        cmd = calloc(1, sizeof(cmd_node));
        cmd->type = close_path;
        append(l, cmd);
        break;

      default:
        exit(1);
    }
  }
  cmd = calloc(1, sizeof(cmd_node));
  cmd->type = fill_and_stroke;
  append(l, cmd);
}

void emit_draw_commands(cmd_list *l, xml_node *node) {
  int has_tranform = 0;

  for (attr_node *p = node->attrs; p; p = p->next) {
    if (strcmp(p->name.str, "fill") == 0) {
      cmd_node *cmd = calloc(1, sizeof(cmd_node));
      cmd->type = fill_color;
      cmd->args.fill_color = parse_color(p->value.str);
      append(l, cmd);
    } else if (strcmp(p->name.str, "stroke") == 0) {
      cmd_node *cmd = calloc(1, sizeof(cmd_node));
      cmd->type = stroke_color;
      cmd->args.stroke_color = parse_color(p->value.str);
      append(l, cmd);
    } else if (strcmp(p->name.str, "stroke-width") == 0) {
      cmd_node *cmd = calloc(1, sizeof(cmd_node));
      cmd->type = stroke_width;
      sscanf(p->value.str, "%f", &cmd->args.stroke_width);
      append(l, cmd);
    } else if (strcmp(p->name.str, "transform") == 0) {
      has_tranform = 1;
      compile_matrix(l, p->value.str);
    }
  }

  if (strcmp(node->tag.str, "path") == 0) {
    for (attr_node *p = node->attrs; p; p = p->next) {
      if (strcmp(p->name.str, "d") == 0) {
        compile_path(l, p->value.str);
      }
    }
  }

  cmd_node *cmd;
  for (xml_node *p = node->children; p; p = p->next) {
    cmd = calloc(1, sizeof(cmd_node));
    cmd->type = save;
    append(l, cmd);

    emit_draw_commands(l, p);

    cmd = calloc(1, sizeof(cmd_node));
    cmd->type = restore;
    append(l, cmd);
  }

  if (has_tranform) {
    cmd_node *cmd = calloc(1, sizeof(cmd_node));
    cmd->type = pop_matrix;
    append(l, cmd);
  }
}

void print_draw_commands(cmd_list *l) {
  for (cmd_node *cmd = l->head; cmd; cmd = cmd->next) {
    printf("%d ", cmd->type);
    switch (cmd->type) {
      case save:
        printf("save\n");
        break;
      case restore:
        printf("restore\n");
        break;
      case stroke_width:
        printf("stroke_width\n%f\n", cmd->args.stroke_width);
        break;
      case stroke_color:
        printf("stroke_color\n%#x\n", cmd->args.stroke_color);
        break;
      case fill_color:
        printf("fill_color\n%#x\n", cmd->args.fill_color);
        break;
      case push_matrix:
        printf("push_matrix\n%f %f %f %f %f %f\n", cmd->args.matrix.a,
               cmd->args.matrix.b, cmd->args.matrix.c, cmd->args.matrix.d,
               cmd->args.matrix.e, cmd->args.matrix.f);
        break;
      case pop_matrix:
        printf("pop_matrix\n");
        break;
      case begin_path:
        printf("begin_path\n");
        break;
      case move_to:
        printf("move_to\n%f %f\n", cmd->args.path.x, cmd->args.path.y);
        break;
      case move_to_d:
        printf("move_to_d\n%f %f\n", cmd->args.path.x, cmd->args.path.y);
        break;
      case line_to:
        printf("line_to\n%f %f\n", cmd->args.path.x, cmd->args.path.y);
        break;
      case line_to_d:
        printf("line_to_d\n%f %f\n", cmd->args.path.x, cmd->args.path.y);
        break;
      case v_line_to:
        printf("v_line_to\n%f\n", cmd->args.path.y);
        break;
      case v_line_to_d:
        printf("v_line_to_d\n%f\n", cmd->args.path.y);
        break;
      case h_line_to:
        printf("h_line_to\n%f\n", cmd->args.path.x);
        break;
      case h_line_to_d:
        printf("h_line_to_d\n%f\n", cmd->args.path.x);
        break;
      case curve_to:
        printf("curve_to\n%f %f %f %f %f %f\n", cmd->args.path.x1,
               cmd->args.path.y1, cmd->args.path.x2, cmd->args.path.y2,
               cmd->args.path.x, cmd->args.path.y);
        break;
      case curve_to_d:
        printf("curve_to_d\n%f %f %f %f %f %f\n", cmd->args.path.x1,
               cmd->args.path.y1, cmd->args.path.x2, cmd->args.path.y2,
               cmd->args.path.x, cmd->args.path.y);
        break;
      case s_curve_to:
        printf("s_curve_to\n%f %f %f %f\n", cmd->args.path.x2,
               cmd->args.path.y2, cmd->args.path.x, cmd->args.path.y);
        break;
      case s_curve_to_d:
        printf("s_curve_to_d\n%f %f %f %f\n", cmd->args.path.x2,
               cmd->args.path.y2, cmd->args.path.x, cmd->args.path.y);
        break;
      case close_path:
        printf("close_path\n");
        break;
      case fill_and_stroke:
        printf("fill_and_stroke\n");
        break;
    }
  }
}

int main() {
  token_stream s;
  read_char(&s.src);
  read_token(&s);

  cmd_list l = {.head = NULL, .tail = NULL};
  xml_node *dom = xml(&s);
  emit_draw_commands(&l, dom);
  print_draw_commands(&l);
  return 0;
}