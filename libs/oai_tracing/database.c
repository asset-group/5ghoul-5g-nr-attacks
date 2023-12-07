#include "database.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef struct {
  char *name;
  char *desc;
  char **groups;
  int  gsize;
  char **arg_type;
  char **arg_name;
  int  asize;
  char *vcd_name;
  int  id;
} id;

typedef struct {
  char *name;
  char **ids;
  int size;
} group;

typedef struct {
  char *name;
  id *i;
  int isize;
  group *g;
  int gsize;
  int *id_to_pos;
} database;

typedef struct {
  char *data;
  int size;
  int maxsize;
} buffer;

typedef struct {
  buffer name;
  buffer value;
} parser;

void put(buffer *b, int c)
{
  if (b->size == b->maxsize) {
    b->maxsize += 256;
    b->data = realloc(b->data, b->maxsize);
    if (b->data == NULL) { printf("memory allocation error\n"); exit(1); }
  }
  b->data[b->size] = c;
  b->size++;
}

void smash_spaces(FILE *f)
{
  int c;
  while (1) {
    c = fgetc(f);
    if (isspace(c)) continue;
    if (c == ' ') continue;
    if (c == '\t') continue;
    if (c == '\n') continue;
    if (c == 10 || c == 13) continue;
    if (c == '#') {
      while (1) {
        c = fgetc(f);
        if (c == '\n' || c == EOF) break;
      }
      continue;
    }
    break;
  }
  if (c != EOF) ungetc(c, f);
}

void get_line(parser *p, FILE *f, char **name, char **value)
{
  int c;
  p->name.size = 0;
  p->value.size = 0;
  *name = NULL;
  *value = NULL;
  smash_spaces(f);
  c = fgetc(f);
  while (!(c == '=' || isspace(c) || c == EOF))
    { put(&p->name, c); c = fgetc(f); }
  if (c == EOF) return;
  put(&p->name, 0);
  while (!(c == EOF || c == '=')) c = fgetc(f);
  if (c == EOF) return;
  smash_spaces(f);
  c = fgetc(f);
  while (!(c == 10 || c == 13 || c == EOF))
    { put(&p->value, c); c = fgetc(f); }
  put(&p->value, 0);
  if (p->name.size <= 1) return;
  if (p->value.size <= 1) return;
  *name = p->name.data;
  *value = p->value.data;
}

int group_cmp(const void *_p1, const void *_p2)
{
  const group *p1 = _p1;
  const group *p2 = _p2;
  return strcmp(p1->name, p2->name);
}

int id_cmp(const void *_p1, const void *_p2)
{
  const id *p1 = _p1;
  const id *p2 = _p2;
  return strcmp(p1->name, p2->name);
}

int string_cmp(const void *_p1, const void *_p2)
{
  char * const *p1 = _p1;
  char * const *p2 = _p2;
  return strcmp(*p1, *p2);
}

id *add_id(database *r, char *idname, int i)
{
  if (bsearch(&(id){name:idname}, r->i, r->isize, sizeof(id), id_cmp) != NULL)
    { printf("ERROR: ID '%s' declared more than once\n", idname); exit(1); }
  if ((r->isize & 1023) == 0) {
    r->i = realloc(r->i, (r->isize + 1024) * sizeof(id));
    if (r->i == NULL) { printf("out of memory\n"); exit(1); }
  }
  r->i[r->isize].name = strdup(idname);
  if (r->i[r->isize].name == NULL) { printf("out of memory\n"); exit(1); }
  r->i[r->isize].desc = NULL;
  r->i[r->isize].groups = NULL;
  r->i[r->isize].gsize = 0;
  r->i[r->isize].arg_type = NULL;
  r->i[r->isize].arg_name = NULL;
  r->i[r->isize].asize = 0;
  r->i[r->isize].vcd_name = NULL;
  r->i[r->isize].id = i;
  r->isize++;
  qsort(r->i, r->isize, sizeof(id), id_cmp);
  return (id*)bsearch(&(id){name:idname}, r->i, r->isize, sizeof(id), id_cmp);
}

group *get_group(database *r, char *group_name)
{
  group *ret;

  ret = bsearch(&(group){name:group_name},
                r->g, r->gsize, sizeof(group), group_cmp);
  if (ret != NULL) return ret;

  if ((r->gsize & 1023) == 0) {
    r->g = realloc(r->g, (r->gsize + 1024) * sizeof(group));
    if (r->g == NULL) abort();
  }
  r->g[r->gsize].name = strdup(group_name);
  if (r->g[r->gsize].name == NULL) abort();
  r->g[r->gsize].ids = NULL;
  r->g[r->gsize].size = 0;
  r->gsize++;

  qsort(r->g, r->gsize, sizeof(group), group_cmp);

  return bsearch(&(group){name:group_name},
                 r->g, r->gsize, sizeof(group), group_cmp);
}

void group_add_id(group *g, char *id)
{
  if ((g->size & 1023) == 0) {
    g->ids = realloc(g->ids, (g->size + 1024) * sizeof(char *));
    if (g->ids == NULL) abort();
  }
  g->ids[g->size] = id;
  g->size++;
}

void id_add_group(id *i, char *group)
{
  char *g = bsearch(&group, i->groups, i->gsize, sizeof(char *), string_cmp);
  if (g != NULL) return;

  if ((i->gsize & 1023) == 0) {
    i->groups = realloc(i->groups, (i->gsize+1024) * sizeof(char *));
    if (i->groups == NULL) abort();
  }
  i->groups[i->gsize] = group;
  i->gsize++;
  qsort(i->groups, i->gsize, sizeof(char *), string_cmp);
}

void add_groups(database *r, id *i, char *groups)
{
  group *g;
  if (i == NULL) {printf("ERROR: GROUP line before ID line\n");exit(1);}
  while (1) {
    char *start = groups;
    char *end = start;
    while (!isspace(*end) && *end != ':' && *end != 0) end++;
    if (end == start) {
      printf("bad group line: groups are seperated by ':'\n");
      abort();
    }
    if (*end == 0) end = NULL; else *end = 0;

    g = get_group(r, start);
    group_add_id(g, i->name);
    id_add_group(i, g->name);

    if (end == NULL) break;
    end++;
    while ((isspace(*end) || *end == ':') && *end != 0) end++;
    if (*end == 0) break;
    groups = end;
  }
}

void add_desc(id *i, char *desc)
{
  if (i == NULL) {printf("ERROR: DESC line before ID line\n");exit(1);}
  i->desc = strdup(desc); if (i->desc == NULL) abort();
}

void add_vcd_name(id *i, char *vcd_name)
{
  if (i == NULL) {printf("ERROR: VCD_NAME line before ID line\n");exit(1);}
  i->vcd_name = strdup(vcd_name); if (i->vcd_name == NULL) abort();
}

char *format_get_next_token(char **cur)
{
  char *start;
  char *end;
  char *ret;

  start = *cur;

  /* remove spaces */
  while (*start && isspace(*start)) start ++;
  if (*start == 0) return NULL;

  /* special cases: ',' and ':' */
  if (*start == ',' || *start == ':') { end = start + 1; goto special; }

  end = start;

  /* go to end of token */
  while (*end && !isspace(*end) && *end != ',' && *end != ':') end++;

special:
  ret = malloc(end-start+1); if (ret == NULL) abort();
  memcpy(ret, start, end-start);
  ret[end-start] = 0;

  *cur = end;
  return ret;
}

void add_format(id *id, char *format)
{
  char *cur = format;
  char *type;
  char *name;
  char *sep;
  while (1) {
    /* parse next type/name: expected " <type> , <name> :" */
    /* get type */
    type = format_get_next_token(&cur);
    if (type == NULL) break;
    /* get comma */
    sep = format_get_next_token(&cur);
    if (sep == NULL || strcmp(sep, ",") != 0) goto error;
    free(sep);
    /* get name */
    name = format_get_next_token(&cur);
    if (name == NULL) goto error;
    /* if there is a next token it has to be : */
    sep = format_get_next_token(&cur);
    if (sep != NULL && strcmp(sep, ":") != 0) goto error;
    free(sep);

    /* add type/name */
    if (id->asize % 64 == 0) {
      id->arg_type = realloc(id->arg_type, (id->asize + 64) * sizeof(char *));
      if (id->arg_type == NULL) abort();
      id->arg_name = realloc(id->arg_name, (id->asize + 64) * sizeof(char *));
      if (id->arg_name == NULL) abort();
    }
    id->arg_type[id->asize] = type;
    id->arg_name[id->asize] = name;
    id->asize++;
  }
  return;

error:
  printf("bad format '%s'\n", format);
  abort();
}

void *parse_database(char *filename)
{
  FILE *in;
  parser p;
  database *r;
  char *name, *value;
  id *last_id = NULL;
  int i;

  r = calloc(1, sizeof(*r)); if (r == NULL) abort();
  memset(&p, 0, sizeof(p));

  r->name = strdup(filename); if (r->name == NULL) abort();

  in = fopen(filename, "r"); if (in == NULL) { perror(filename); abort(); }

  i = 0;

  while (1) {
    get_line(&p, in, &name, &value);
    if (name == NULL) break;
//printf("%s %s\n", name, value);
    if (!strcmp(name, "ID")) { last_id = add_id(r, value, i); i++; }
    if (!strcmp(name, "GROUP")) add_groups(r, last_id, value);
    if (!strcmp(name, "DESC")) add_desc(last_id, value);
    if (!strcmp(name, "FORMAT")) add_format(last_id, value);
    if (!strcmp(name, "VCD_NAME")) add_vcd_name(last_id, value);
  }

  fclose(in);
  free(p.name.data);
  free(p.value.data);

  /* setup id_to_pos */
  r->id_to_pos = malloc(sizeof(int) * r->isize);
  if (r->id_to_pos == NULL) abort();
  for (i = 0; i < r->isize; i++)
    r->id_to_pos[r->i[i].id] = i;

  return r;
}

void dump_database(void *_d)
{
  database *d = _d;
  int i;

  printf("database %s: %d IDs, %d GROUPs\n", d->name, d->isize, d->gsize);
  for (i = 0; i < d->isize; i++) {
    int j;
    printf("ID %s [%s] [in %d group%s]\n",
           d->i[i].name, d->i[i].desc ? d->i[i].desc : "",
           d->i[i].gsize, d->i[i].gsize > 1 ? "s" : "");
    for (j = 0; j < d->i[i].gsize; j++)
      printf("    in GROUP: %s\n", d->i[i].groups[j]);
    if (d->i[i].asize == 0) printf("    no FORMAT\n");
    else {
      int j;
      printf("    FORMAT: ");
      for (j = 0; j < d->i[i].asize; j++)
        printf("'%s' , '%s' %s", d->i[i].arg_type[j], d->i[i].arg_name[j],
               j == d->i[i].asize-1 ? "" : " : ");
      printf("\n");
    }
  }
  for (i = 0; i < d->gsize; i++) {
    int j;
    printf("GROUP %s [size %d]\n", d->g[i].name, d->g[i].size);
    for (j = 0; j < d->g[i].size; j++)
      printf("  contains ID: %s\n", d->g[i].ids[j]);
  }
}

void list_ids(void *_d)
{
  database *d = _d;
  int i;
  for (i = 0; i < d->isize; i++) printf("%s\n", d->i[i].name);
}

void list_groups(void *_d)
{
  database *d = _d;
  int i;
  for (i = 0; i < d->gsize; i++) printf("%s\n", d->g[i].name);
}

static int onoff_id(database *d, char *name, int *a, int onoff)
{
  id *i;
  i = bsearch(&(id){name:name}, d->i, d->isize, sizeof(id), id_cmp);
  if (i == NULL) return 0;
  a[i->id] = onoff;
  // printf("turning %s %s\n", onoff ? "ON" : "OFF", name);
  return 1;
}

static int onoff_group(database *d, char *name, int *a, int onoff)
{
  group *g;
  int i;
  g = bsearch(&(group){name:name}, d->g, d->gsize, sizeof(group), group_cmp);
  if (g == NULL) return 0;
  for (i = 0; i < g->size; i++) onoff_id(d, g->ids[i], a, onoff);
  return 1;
}

void on_off(void *_d, char *item, int *a, int onoff)
{
  int done;
  database *d = _d;
  int i;
  if (item == NULL) {
    for (i = 0; i < d->isize; i++) a[i] = onoff;
    // printf("turning %s all traces\n", onoff ? "ON" : "OFF");
    return;
  }
  done = onoff_group(d, item, a, onoff);
  done += onoff_id(d, item, a, onoff);
  if (done == 0) {
    printf("ERROR: ID/group '%s' not found in database\n", item);
    exit(1);
  }
}

char *event_name_from_id(void *_database, int id)
{
  database *d = _database;
  return d->i[d->id_to_pos[id]].name;
}

char *event_vcd_name_from_id(void *_database, int id)
{
  database *d = _database;
  return d->i[d->id_to_pos[id]].vcd_name;
}

int event_id_from_name(void *_database, char *name)
{
  database *d = _database;
  id *i = (id*)bsearch(&(id){name:name}, d->i, d->isize, sizeof(id), id_cmp);
  if (i == NULL)
    { printf("%s:%d: '%s' not found\n", __FILE__, __LINE__, name); abort(); }
  return i->id;
}

database_event_format get_format(void *_database, int event_id)
{
  database *d = _database;
  database_event_format ret;

  if (event_id < 0 || event_id >= d->isize) {
    printf("%s:%d: bad event ID (%d)\n", __FILE__, __LINE__, event_id);
    abort();
  }

  ret.type = d->i[d->id_to_pos[event_id]].arg_type;
  ret.name = d->i[d->id_to_pos[event_id]].arg_name;
  ret.count = d->i[d->id_to_pos[event_id]].asize;

  return ret;
}

int number_of_ids(void *_d)
{
  database *d = _d;
  return d->isize;
}

int database_get_ids(void *_d, char ***ids)
{
  database *d = _d;
  int i;
  *ids = malloc(d->isize * sizeof(char **)); if (*ids == NULL) abort();
  for (i = 0; i < d->isize; i++) (*ids)[i] = d->i[i].name;
  return d->isize;
}

int database_get_groups(void *_d, char ***groups)
{
  database *d = _d;
  int i;
  *groups = malloc(d->gsize * sizeof(char **)); if (*groups == NULL) abort();
  for (i = 0; i < d->gsize; i++) (*groups)[i] = d->g[i].name;
  return d->gsize;
}

int database_pos_to_id(void *_d, int pos)
{
  database *d = _d;
  return d->i[pos].id;
}

void database_get_generic_description(void *_d, int id,
    char **name, char **desc)
{
  database *d = _d;
  int pos = d->id_to_pos[id];
  OBUF o;
  int i;
  *name = strdup(d->i[pos].name); if (*name == NULL) abort();
  o.osize = o.omaxsize = 0;
  o.obuf = NULL;
  PUTS(&o, *name);
  for (i = 0; i < d->i[pos].asize; i++) {
    PUTC(&o, ' ');
    PUTS(&o, d->i[pos].arg_name[i]);
    PUTS(&o, " [");
    PUTS(&o, d->i[pos].arg_name[i]);
    PUTS(&o, "]");
  }
  PUTC(&o, 0);
  *desc = o.obuf;
}
