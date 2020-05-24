#include "hashdata.h"

struct HashTable *hashtable_create(size_t size) {
	struct HashTable *new_hashtable = malloc(sizeof(*new_hashtable));
	if (new_hashtable == NULL) {
		fprintf(stderr, "Unable to allocate memory for HashTable.\n");
		return NULL;
	}

	new_hashtable->size = size;

	new_hashtable->table = calloc(size, sizeof(*new_hashtable->table));
	if (new_hashtable->table == NULL) {
		free(new_hashtable);
		fprintf(stderr, "Unable to allocate memory for HashTable.\n");
		return NULL;
	}

	return new_hashtable;
}

void hashtable_destroy(struct HashTable *hashtable) {
	size_t i;
	struct HashTableDefinition *next, *curr;

	for (i = 0; i < hashtable->size; i++) {
		curr = hashtable->table[i];
		while (curr != NULL) {
			next = curr->next;
			free(curr);
			curr = next;
		}
	}

	free(hashtable->table);
	free(hashtable);
}

bool hashtable_store(struct HashTable *hashtable, char *key, union HashTableValue value,
					 enum HashTableType type) {
	// Calculate key hash and determine table position
	uint32_t hash = __djb2_a(key);
	uint32_t table_pos = hash % hashtable->size;
	struct HashTableDefinition *data_pos = hashtable->table[table_pos];
	bool assign_head = data_pos == NULL;

	// Traverse through linked list
	while (data_pos != NULL) {
		if (strcmp(data_pos->key, key) == 0) {
			data_pos->value = value;
			return true;
		} else {
			data_pos = data_pos->next;
		}
	}

	// Create new item if end of linked list is found
	struct HashTableDefinition *new_pos = malloc(sizeof(*new_pos));
	if (new_pos == NULL) {
		return false;
	}

	new_pos->key = key;
	new_pos->value = value;
	new_pos->type = type;
	new_pos->next = NULL;

	// Assign at position
	if (assign_head) {
		hashtable->table[table_pos] = new_pos;
	} else {
		data_pos->next = new_pos;
	}

	return true;
}

bool hashtable_access(struct HashTable *hashtable, char *key, union HashTableValue *result_ptr) {
	uint32_t hash = __djb2_a(key);
	int table_pos = hash % hashtable->size;
	struct HashTableDefinition *data_pos = hashtable->table[table_pos];

	while (data_pos != NULL) {
		if (strcmp(data_pos->key, key) == 0) {
			if (result_ptr != NULL)
				*result_ptr = data_pos->value;
			return true;
		}
		data_pos = data_pos->next;
	}

	return false;
}

uint32_t __djb2_a(char *key) {
	uint32_t hash = 0x1505;
	int i;

	while ((i = *key++)) {
		hash = (33 * hash) ^ i;
	}

	return hash;
}