#include "ruby.h"
#include <string.h>
#define NUM_CHILDREN 256

typedef struct _trie Trie;

struct _trie {
    char c;
    unsigned long count;
    Trie *children[NUM_CHILDREN];
    VALUE value;
};

/***** Trie Operation *****/
static Trie*
trie_new(void)
{
    Trie *trie = ALLOC(Trie);
    trie->c = 0;
    trie->count = 0;
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        trie->children[i] = NULL;
    }
    trie->value = 0;
    return trie;
}

static void
trie_free(Trie *trie)
{
    if (trie) {
        for (unsigned int i = 0; i < NUM_CHILDREN; ++i) {
            trie_free(trie->children[i]);
        }
        free(trie);
    }
}

static Trie*
trie_search(Trie* trie, const char *key)
{
    Trie *node = trie;
    const size_t key_len = strlen(key);
    for (unsigned long i = 0; i < key_len; ++i) {
        const int code = (int)key[i];
        if (!node->children[code]) {
            return NULL;
        }
        node = node->children[code];
    }
    // node is found but it is not assigned.
    return node;
}

static Trie*
trie_search_leaf(Trie* trie, const char *key)
{
    Trie* result = trie_search(trie, key);
    if (!result || !result->value)
        return NULL;
    return result;
}

static void
trie_traverse(Trie* trie, Trie **result)
{
    if (trie) {
        unsigned long last;
        for (last = 0; result[last] != NULL; ++last);
        if (trie->value) {
            result[last] = trie;
        }
        for (int i = 0; i < NUM_CHILDREN; ++i) {
            trie_traverse(trie->children[i], result);
        }
    }
}

/***** Ruby Method Implementation *****/
/*** Allocation function ***/
static VALUE
trie_alloc(VALUE self)
{
    return Data_Wrap_Struct(self, 0, trie_free, trie_new());
}

/***
 * Trie#store(key, value)
 * Associates the *value* with the *key*. *key* must be a String.
 * Returns *value*.
***/
static VALUE
trie_store(VALUE self, VALUE key, VALUE value)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    const char *key_str = StringValuePtr(key);
    const size_t key_len = strlen(key_str);
    if (0 == key_len) {
        ++trie->count;
        trie->value = value;
    } else {
        Trie *node = trie;
        for (unsigned long i = 0; i < key_len; ++i) {
            const int code = (int)key_str[i];
            if (!node->children[code]) {
                node->children[code] = trie_new();
                node->children[code]->c = (char)code;
            }
            ++node->count;
            node = node->children[code];
        }
        node->value = value;
    }
    return value;
}

/***
 * Trie#get(key)
 * Gets the value associated with *key*.
 * Returns the value associated with *key* if found; otherwise, nil.
***/
static VALUE
trie_get(VALUE self, VALUE key)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    const char *key_str = StringValuePtr(key);
    Trie *result = trie_search_leaf(trie, key_str);
    if (!result)
        return Qnil;
    return result->value;
}

/***
 * Trie#each { |value| ... }
 * Evaluates the given block with the values in this tree.
 * The order of evaluation is dictionary order.
 * The block must be given.
***/
static VALUE
trie_each(VALUE self)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    Trie **result = (Trie **)malloc(sizeof(Trie *) * trie->count);
    for (unsigned long i = 0; i < trie->count; ++i) {
        result[i] = 0;
    }
    trie_traverse(trie, result);
    for (unsigned long i = 0; i < trie->count; ++i) {
        rb_yield(result[i]->value);
    }
    free(result);
    return self;
}

/***
 * Trie#common_prefix_each { |value| ... }
 * Evaluates the given block with the values that has the common prefix in this tree.
 * The order of evaluation is dictionary order.
 * The block must be given.
***/
static VALUE
trie_common_prefix_each(VALUE self, VALUE key)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    const char* key_str = StringValuePtr(key);
    Trie *sub = trie_search(trie, key_str);
    if (!sub) {
        return self;
    }
    unsigned long sub_count = sub->count;
    if (sub->value) {
        ++sub_count;
    }
    Trie **result = (Trie **)malloc(sizeof(Trie *) * sub_count);
    for (unsigned long i = 0; i < sub_count; ++i) {
        result[i] = NULL;
    }
    trie_traverse(sub, result);
    for (unsigned long i = 0; i < sub_count; ++i) {
        rb_yield(result[i]->value);
    }
    free(result);
    return self;
}

/***
 * Trie#size
 * Trie#length
 * Get the number of the elements in this tree.
***/
static VALUE
trie_size(VALUE self)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    return ULONG2NUM(trie->count);
}

void
Init_trie(void)
{
    VALUE cTrie = rb_define_class("Trie", rb_cObject);
    rb_include_module(cTrie, rb_mEnumerable);
    rb_define_alloc_func(cTrie, trie_alloc);
    rb_define_method(cTrie, "store", trie_store, 2);
    rb_define_method(cTrie, "[]=", trie_store, 2);
    rb_define_method(cTrie, "get", trie_get, 1);
    rb_define_method(cTrie, "[]", trie_get, 1);
    rb_define_method(cTrie, "each", trie_each, 0);
    rb_define_method(cTrie, "common_prefix_each", trie_common_prefix_each, 1);
    rb_define_method(cTrie, "size", trie_size, 0);
    rb_define_method(cTrie, "length", trie_size, 0);
}

// vim: set expandtab ts=4 sw=4 sts=4:
