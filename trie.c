#include "ruby.h"
#include <string.h>
#define NUM_CHILDREN 256
#define INITIAL_STR_LEN 32
#define INCREMENT_STR_LEN 32

typedef struct _trie Trie;
struct _trie {
    char c;
    Trie *next;
    Trie *child;
    VALUE value;
};

typedef VALUE (*TRAVERSE_FUNC)(const Trie *node);
typedef VALUE (*TRAVERSE_WITH_KEY_FUNC)(const Trie *node, const char *key);

/***** Trie Operation *****/
static Trie*
trie_new(void)
{
    Trie *trie = ALLOC(Trie);
    trie->c = 0;
    trie->next = NULL;
    trie->child = NULL;
    trie->value = 0;
    return trie;
}

static void
trie_free(Trie *trie)
{
    if (trie) {
        if (trie->next) {
            trie_free(trie->next);
        }
        if (trie->child) {
            trie_free(trie->child);
        }
        ruby_xfree(trie);
    }
}

static Trie *
trie_add_child(Trie *trie, Trie *child)
{
    if (trie->child) {
        child->next = trie->child;
        trie->child = child;
    } else {
        trie->child = child;
    }
    return child;
}

static Trie *
trie_find(Trie *start, const char key)
{
    Trie *node;
    for (node = start; node != NULL && node->c != key; node = node->next);
    return node;
}

static Trie *
trie_create(Trie *trie, const char *key)
{
    const size_t key_len = strlen(key);
    Trie *node = trie;
    for (size_t i = 0; i < key_len; ++i) {
        const char k = key[i];
        Trie *child = trie_find(node->child, k);
        if (!child) {
            child = trie_new();
            child->c = k;
            trie_add_child(node, child);
        }
        node = child;
    }
    return node;
}

static Trie*
trie_search(Trie* trie, const char *key)
{
    Trie *node = trie;
    const size_t key_len = strlen(key);
    for (size_t i = 0; i < key_len; ++i) {
        const char k = key[i];
        if (node->c == k) {
            node = trie_find(node->next, k);
        } else {
            node = trie_find(node->child, k);
        }
    }
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
trie_traverse(Trie* trie, TRAVERSE_FUNC func)
{
    if (trie) {
        if (trie->value) {
            func(trie);
        }
        for (Trie *node = trie->child; node != NULL; node = node->next) {
            trie_traverse(node, func);
        }
    }
}

static void
trie_traverse_with_key(Trie *trie, const char *key, TRAVERSE_WITH_KEY_FUNC func)
{
    if (trie) {
        const size_t key_len = strlen(key);
        char *tmp_key = ALLOC_N(char, key_len + 2);
        strcpy(tmp_key, key);
        tmp_key[key_len] = trie->c;
        if (trie->value) {
            func(trie, tmp_key);
        }
        for (Trie *node = trie->child; node != NULL; node = node->next) {
            trie_traverse_with_key(node, tmp_key, func);
        }
    }
}

static unsigned long
trie_count(Trie *trie)
{
    unsigned long count = 0;
    if (trie->value) {
        ++count;
    }
    for (Trie *node = trie->child; node != NULL; node = node->next) {
        count += trie_count(node);
    }
    return count;
}

/***** Helper function *****/
static VALUE
trie_traverse_func(const Trie *node)
{
    return rb_yield(node->value);
}

static VALUE
trie_traverse_with_key_func(const Trie *node, const char* key)
{
    return rb_yield(rb_ary_new3(2, rb_str_new2(key), node->value));
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
    Trie *node = trie_create(trie, StringValuePtr(key));
    node->value = value;
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
    trie_traverse_with_key(trie, "", trie_traverse_with_key_func);
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
    trie_traverse(sub, trie_traverse_func);
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
    return ULONG2NUM(trie_count(trie));
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
