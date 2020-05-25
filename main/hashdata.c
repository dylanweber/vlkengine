#include "hashdata.h"

/**
 * @brief Creates an empty HashTable of size 'size'
 *
 * Must be destroyed with 'hashtable_destroy'
 *
 * @param size Size to make the HashTable
 * @return struct HashTable* Created HashTable
 */
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

/**
 * @brief Destroys a HashTable and all it's definitions
 *
 * @param hashtable HashTable to destroy
 */
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

/**
 * @brief Inserts an item into the HashTable
 *
 * @param hashtable HashTable to insert into
 * @param key Key string to reference with
 * @param value Value union to store
 * @param type (optional) Type of value
 * @return true Successfully stored item
 * @return false Failed to store item
 */
bool hashtable_store(struct HashTable *hashtable, char *key, union HashTableValue value,
					 enum HashTableType type) {
	// Calculate key hash and determine table position
	uint32_t hash = __djb2_a(key);
	uint32_t table_pos = hash % hashtable->size;
	struct HashTableDefinition *data_pos = hashtable->table[table_pos], *prev_pos;
	bool assign_head = data_pos == NULL;

	// Traverse through linked list
	while (data_pos != NULL) {
		if (strcmp(data_pos->key, key) == 0) {
			data_pos->value = value;
			return true;
		} else {
			prev_pos = data_pos;
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
		prev_pos->next = new_pos;
	}

	return true;
}

/**
 * @brief Checks if key exists in HashTable
 *
 * @param hashtable HashTable to check
 * @param key Key to reference with
 * @return true Item is found
 * @return false Item is not found
 */
bool hashtable_exists(struct HashTable *hashtable, char *key) {
	// Calculate key hash and determine table position
	uint32_t hash = __djb2_a(key);
	uint32_t table_pos = hash % hashtable->size;
	struct HashTableDefinition *data_pos = hashtable->table[table_pos];

	// Traverse through linked list
	while (data_pos != NULL) {
		if (strcmp(data_pos->key, key) == 0) {
			return true;
		} else {
			data_pos = data_pos->next;
		}
	}

	return false;
}

/**
 * @brief Retrieves a value from the HashTable
 *
 * @param hashtable HashTable to get the value from
 * @param key Key to reference with
 * @param result_ptr Pointer to a position to store the result
 * @return true Item is found
 * @return false Item is not found
 */
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

/**
 * @brief Creates an empty HashSet of size 'size'
 *
 * Must be destroyed with 'hashset_destroy'
 *
 * @param size Size to make the HashSet
 * @return struct HashSet* Created HashSet
 */
struct HashSet *hashset_create(size_t size) {
	struct HashSet *new_hashset = malloc(sizeof(*new_hashset));
	if (new_hashset == NULL) {
		fprintf(stderr, "Failure to allocate memory for HashSet.\n");
		return NULL;
	}

	new_hashset->size = size;

	new_hashset->table = calloc(size, sizeof(*new_hashset->table));
	if (new_hashset->table == NULL) {
		free(new_hashset);
		fprintf(stderr, "Failure to allocate memory for HashSet table.\n");
		return NULL;
	}

	return new_hashset;
}

/**
 * @brief Destroys a HashSet and all it's definitions
 *
 * @param hashset HashSet to destroy
 */
void hashset_destroy(struct HashSet *hashset) {
	size_t i;
	struct HashSetDefinition *next, *curr;

	for (i = 0; i < hashset->size; i++) {
		curr = hashset->table[i];
		while (curr != NULL) {
			next = curr->next;
			free(curr);
			curr = next;
		}
	}

	free(hashset->table);
	free(hashset);
}

/**
 * @brief Inserts an item into the HashSet
 *
 * @param hashset HashSet to insert into
 * @param key Key to reference with
 * @return true Key has been stored correctly
 * @return false Key could not be stored
 */
bool hashset_store(struct HashSet *hashset, const char *key) {
	// Calculate key hash and determine table position
	uint32_t hash = __djb2_a(key);
	uint32_t table_pos = hash % hashset->size;
	struct HashSetDefinition *data_pos = hashset->table[table_pos], *prev_pos;
	bool assign_head = data_pos == NULL;

	// Traverse through linked list
	while (data_pos != NULL) {
		if (strcmp(data_pos->key, key) == 0) {
			return true;
		} else {
			prev_pos = data_pos;
			data_pos = data_pos->next;
		}
	}

	// Create new item if end of linked list is found
	struct HashSetDefinition *new_pos = malloc(sizeof(*new_pos));
	if (new_pos == NULL) {
		return false;
	}

	new_pos->key = key;
	new_pos->next = NULL;

	// Assign at position
	if (assign_head) {
		hashset->table[table_pos] = new_pos;
	} else {
		prev_pos->next = new_pos;
	}

	return true;
}

/**
 * @brief Checks if an string exists in the set
 *
 * @param hashset Set to check
 * @param key String to reference with
 * @return true String has been found
 * @return false String has not been found
 */
bool hashset_exists(struct HashSet *hashset, const char *key) {
	// Calculate key hash and determine table position
	uint32_t hash = __djb2_a(key);
	uint32_t table_pos = hash % hashset->size;
	struct HashSetDefinition *data_pos = hashset->table[table_pos];

	// Traverse through linked list
	while (data_pos != NULL) {
		if (strcmp(data_pos->key, key) == 0) {
			return true;
		} else {
			data_pos = data_pos->next;
		}
	}

	return false;
}

/**
 * @brief Prints a set
 *
 * @param hashset Set to print
 */
void hashset_print(struct HashSet *hashset) {
	size_t i;
	struct HashSetDefinition *curr;

	printf("{");

	for (i = 0; i < hashset->size; i++) {
		curr = hashset->table[i];
		while (curr != NULL) {
			printf("\"%s\",", curr->key);
			curr = curr->next;
		}
	}

	printf("\b}\n");
}

/**
 * @brief Hash function for HashTable and HashSet
 *
 * @param key Input string for hash
 * @return uint32_t Output hash
 */
uint32_t __djb2_a(const char *key) {
	uint32_t hash = 0x1505;
	int i;

	while ((i = *key++)) {
		hash = (33 * hash) ^ i;
	}

	return hash;
}