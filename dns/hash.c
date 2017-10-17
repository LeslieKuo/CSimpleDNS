#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

/*
 *  hash() - hash algorithm function
 *
 *  tptr: hash table
 *  key: key
 */
static int hash(const hash_t *tptr, const char *key) 
{
    unsigned int hash = 0;  
   
    while (*key)  
    {   
		//hash<<5-hash: hash*31; The seed can be: 31, 131, 1313 etc...
        hash = ((hash << 5) - hash) + (*key++);  
    }  
   
    return (hash & tptr->size);  

}

/*
 *  hash_init() - initiate the hash table.
 *
 *  tptr: pointer to hash struct
 *  buckets: number of buckets in hash table
 */
void hash_init(hash_t *tptr, int buckets) 
{

  /* default 16 buckets */
  if (buckets==0)
    buckets=16;

  tptr->size=buckets;

  tptr->bucket=(hash_node_t **) calloc(tptr->size, sizeof(hash_node_t *));

  return;
}


/*
 *  hash_lookup() - look up function, return HASH_FAIL when lookup failed
 *
 *  tptr:  hash table
 *  key:   key
 */
int hash_lookup(const hash_t *tptr, const char *key) 
{
  int h;
  hash_node_t *node;

  /* obtain value of corresponding key*/
  h=hash(tptr, key);
  for (node=tptr->bucket[h]; node!=NULL; node=node->next)
  {
    if (0 == strcmp(node->key, key))
      break;
  }

  /* return value, or HASH_FAIL */
  return(node ? node->data : HASH_FAIL);
}

/*
 *  hash_insert() - insert data into hash table
 *
 *  tptr: hash table
 *  key: key
 *  data: data
 */
int hash_insert(hash_t *tptr, const char *key, int data) 
{
  int tmp;
  hash_node_t *node;
  int h;

  /* If already exist, return the node value directly*/
  if ((tmp=hash_lookup(tptr, key)) != HASH_FAIL)
    return(tmp);

  /*insert the item to bucket end*/
  h=hash(tptr, key);
  node=(struct hash_node_t *) malloc(sizeof(hash_node_t));
  node->data=data;
  node->key=(char*)malloc(strlen(key)+1);
  strcpy(node->key, key);
  node->key[strlen(key)] = '\0';
  node->next=tptr->bucket[h];
  tptr->bucket[h]=node;
  tptr->entries++;

  return HASH_OK;
}


/*
 * hash_destroy() -  free hash table
 * 
 */
void hash_destroy(hash_t *tptr) 
{
  hash_node_t *node, *last;
  int i;

  for (i=0; i<tptr->size; i++) 
  {
    node = tptr->bucket[i];
    while (node != NULL)
    { 
      last = node;   
      node = node->next;
      free(last->key);
      free(last);
    }
  }     

  /* free bucket data */
  if (tptr->bucket != NULL) 
  {
    free(tptr->bucket);
    memset(tptr, 0, sizeof(hash_t));
  }
}

