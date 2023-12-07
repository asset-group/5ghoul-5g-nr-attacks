#ifndef _DATABASE_H_
#define _DATABASE_H_

/* returns an opaque pointer - truly a 'database *', see database.c */
void *parse_database(char *filename);
void dump_database(void *database);
void list_ids(void *database);
void list_groups(void *database);
void on_off(void *d, char *item, int *a, int onoff);
char *event_name_from_id(void *database, int id);
char *event_vcd_name_from_id(void *_database, int id);
int event_id_from_name(void *database, char *name);
int number_of_ids(void *database);
int database_get_ids(void *database, char ***ids);
int database_get_groups(void *database, char ***groups);
int database_pos_to_id(void *database, int pos);
void database_get_generic_description(void *database, int id,
    char **name, char **desc);

/****************************************************************************/
/* get format of an event                                                   */
/****************************************************************************/

typedef struct {
  char **type;
  char **name;
  int count;
} database_event_format;

database_event_format get_format(void *database, int event_id);

#endif /* _DATABASE_H_ */
