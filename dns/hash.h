
typedef struct hash_node_t {
  int data;                           /* data in hash node */
  char * key;                   /* key for hash lookup */
  struct hash_node_t *next;           /* next node in hash chain */
} hash_node_t;

typedef struct hash_t 
{
  struct hash_node_t **bucket;        /* hash bucket node */
  int size;                           /* number of buckets */
  int entries;                        /* total number of items */
} hash_t;

#define HASH_FAIL -1
#define HASH_OK 0

void hash_init(hash_t *, int);

int hash_lookup (const hash_t *, const char *);

int hash_insert (hash_t *, const char *, int);

void hash_destroy(hash_t *);


