#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dict.h"

struct dict_item {
  // Each word is at most 100 bytes.
  char word[100];
  // The length of each word.
  size_t len;
};

struct dict_t {
  // The path to the underlying file
  char *path;
  // The file descriptor of the opened file. Set by dictionary_open_map.
  int fd;
  // How many items the mmaped file should store (this will determine the size).
  // There are ~500k words in /usr/share/dict/words.
  size_t num_items;
  // A pointer to the first item.
  struct dict_item *base;
};

// Construct a new dict_t struct.
// data_file is where to write the data,
// num_items is how many items this data file should store.
struct dict_t* dictionary_new(char *data_file, size_t num_items) {
  struct dict_t* dict = malloc(sizeof(struct dict_t));
  dict->path = data_file;
  dict->num_items = num_items;
  return dict;
}

// Computes the size of the underlying file based on the # of items and the size
// of a dict_item.
size_t dictionary_len(struct dict_t *dict) {
  return dict->num_items * sizeof(struct dict_item);
}

// This is a private helper function. It should:
// Open the underlying path (dict->path), ftruncate it to the appropriate length
// (dictionary_len), then mmap it.
int dictionary_open_map(struct dict_t *dict) {
  dict->fd = open(dict->path, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR);
  if (dict->fd == -1) {
    perror("open");
    return EXIT_FAILURE;
  }
  
  dict->num_items = dictionary_len(dict);
  if (ftruncate(dict->fd, dict->num_items) == -1) {
    perror("ftruncate");
    return EXIT_FAILURE;
  }

  dict->base = mmap(NULL, dict->num_items, PROT_READ | PROT_WRITE, MAP_SHARED, dict->fd, 0);
  if (dict->base == MAP_FAILED) {
    perror("mmap");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// The rest of the functions should be whatever is left from the header that
// hasn't been defined yet.
// Good luck


// Read the file at input. For each line in input, create a new dictionary
// entry. 
int dictionary_generate(struct dict_t *dict, char *input) {
  dict->path = input;
  dictionary_open_map(dict);
  FILE* file = fopen(dict->path, "r");
  char line [100];
  int i = 0;
  while (fgets(line, 100, file) != NULL) {
    if (strcmp(line, "\n") == 0) {
      strcpy(dict->base[i].word, "");
    } else {
      strcpy(dict->base[i].word, strtok(line, "\n"));
    }
    i++;
  }
  fclose(file);
  return EXIT_SUCCESS;
}


// load a dictionary that was generated by calling generate.
int dictionary_load(struct dict_t *dict) {
  return dictionary_open_map(dict);
}

// Unmaps the given dictionary.
// Free/destroy the underlying dict. Does NOT delete the database file.
void dictionary_close(struct dict_t *dict) {
  msync(dict, dict->num_items, MS_SYNC);
  munmap(dict, dict->num_items);
}

// returns pointer to word if it exists, null otherwise
char* dictionary_exists(struct dict_t *dict, char *word) {
  for (int i = 0; i < dict->num_items; i++) {
    if (strcmp(dict->base[i].word, word) == 0) {
      return dict->base[i].word;
    }
  }
  return NULL;
}

// Count of words with len > n
int dictionary_larger_than(struct dict_t *dict, size_t n) {
  int c = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (strlen(dict->base[i].word) > n) {
      c++;
    }
  }
  return c;
}

// Count of words with len < n
int dictionary_smaller_than(struct dict_t *dict, size_t n) {
  int c = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (strlen(dict->base[i].word) < n) {
      c++;
    }
  }
  return c;
}

// Count of words with len == n
int dictionary_equal_to(struct dict_t *dict, size_t n) {
  int c = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (strlen(dict->base[i].word) == n) {
      c++;
    }
  }
  return c;
}
