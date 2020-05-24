#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HASHDATA_H
#define HASHDATA_H

enum HashTableType {
	HASHTABLE_UNSPEC = 0,
	HASHTABLE_PTR,
	HASHTABLE_INT,
	HASHTABLE_FLOAT,
	HASHTABLE_DOUBLE,
	HASHTABLE_CHAR,
	HASHTABLE_STRING
};

union HashTableValue {
	void *ptr;
	int i;
	float f;
	double d;
	char c;
	char *s;
};

struct HashTable {
	struct HashTableDefinition **table;
	size_t size;
};

struct HashTableDefinition {
	char *key;
	union HashTableValue value;
	enum HashTableType type;
	struct HashTableDefinition *next;
};

struct HashSet {
	struct HashSetDefinition **table;
	size_t size;
};

struct HashSetDefinition {
	char *key;
	struct HashSetDefinition *next;
};

struct HashTable *hashtable_create(size_t size);
void hashtable_destroy(struct HashTable *);
bool hashtable_store(struct HashTable *, char *, union HashTableValue, enum HashTableType);
bool hashtable_exists(struct HashTable *, char *);
bool hashtable_access(struct HashTable *, char *, union HashTableValue *);

struct HashSet *hashset_create(size_t size);
void hashset_destroy(struct HashSet *);
bool hashset_store(struct HashSet *, char *);
bool hashset_exists(struct HashSet *, char *);

uint32_t __djb2_a(char *);

#endif