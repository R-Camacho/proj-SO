#ifndef KEY_VALUE_STORE_H
#define KEY_VALUE_STORE_H

#define TABLE_SIZE 26

#include <pthread.h>
#include <stddef.h>

typedef struct KeyNode {
  char *key;
  char *value;
  struct KeyNode *next;
} KeyNode;

typedef struct HashTable {
  KeyNode *table[TABLE_SIZE];
} HashTable;

/// Creates a new event hash table.
/// @return Newly created hash table, NULL on failure
struct HashTable *create_hash_table();

/// Appends a new key value pair to the hash table.
/// @param ht Hash table to be modified.
/// @param key Key of the pair to be written.
/// @param value Value of the pair to be written.
/// @return 0 if the node was appended successfully, 1 otherwise.
int write_pair(HashTable *ht, const char *key, const char *value);

/// Reads the value of given key.
/// @param ht Hash table to search.
/// @param key Key of the pair to read.
/// @return the key value if the key was found, NULL otherwise.
char *read_pair(HashTable *ht, const char *key);

/// Deletes the value of given key.
/// @param ht Hash table to delete from.
/// @param key Key of the pair to be deleted.
/// @return 0 if the node was deleted successfully, 1 otherwise.
int delete_pair(HashTable *ht, const char *key);

/// Frees the hashtable.
/// @param ht Hash table to be deleted.
void free_table(HashTable *ht);

/// Find a node in the hash table.
/// @param ht Hash table to search.
/// @param key Key to search for.
/// @return KeyNode if found, NULL otherwise.
KeyNode *find_node(HashTable *ht, char *key);

#endif // KVS_H
