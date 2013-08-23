#include "ruby.h"
#include <string.h>
#define NUM_CHILDREN 256
#define INITIAL_STR_LEN 32
#define INCREMENT_STR_LEN 32

#define IS_MODIFIABLE(trie) (0 == trie->traversing)

typedef struct _trie Trie;
typedef struct _trienode TrieNode;

struct _trie {
    TrieNode *root;
    int traversing;
};

struct _trienode {
    char c;
    TrieNode *next;
    TrieNode *child;
    VALUE value;
};

typedef VALUE (*TRAVERSE_FUNC)(const TrieNode *node);
typedef VALUE (*TRAVERSE_WITH_KEY_FUNC)(const TrieNode *node, const char *key);

/***** Trie Operation *****/
static TrieNode *
trienode_new(void)
{
    TrieNode *node = ALLOC(TrieNode);
    node->c = 0;
    node->next = NULL;
    node->child = NULL;
    node->value = 0;
    return node;
}

static void
trienode_free(TrieNode *node)
{
    if (node) {
        if (node->next) {
            trienode_free(node->next);
        }
        if (node->child) {
            trienode_free(node->child);
        }
        ruby_xfree(node);
    }
}

static TrieNode *
trienode_add_child(TrieNode *node, TrieNode *child)
{
    if (node->child) {
        child->next = node->child;
        node->child = child;
    } else {
        node->child = child;
    }
    return child;
}

static TrieNode *
trienode_find(TrieNode *start, const char key)
{
    TrieNode *p;
    for (p = start; p != NULL && p->c != key; p = p->next);
    return p;
}

static TrieNode *
trienode_create(TrieNode *node, const char *key)
{
    const size_t key_len = strlen(key);
    TrieNode *p = node;
    for (size_t i = 0; i < key_len; ++i) {
        const char k = key[i];
        TrieNode *child = trienode_find(p->child, k);
        if (!child) {
            child = trienode_new();
            child->c = k;
            trienode_add_child(p, child);
        }
        p = child;
    }
    return p;
}

static TrieNode *
trienode_search(TrieNode* node, const char *key)
{
    TrieNode *p = node;
    const size_t key_len = strlen(key);
    for (size_t i = 0; i < key_len; ++i) {
        const char k = key[i];
        if (p->c == k) {
            p = trienode_find(p->next, k);
        } else {
            p = trienode_find(p->child, k);
        }
        if (!p) {
            return NULL;
        }
    }
    return p;
}

static TrieNode *
trienode_search_leaf(TrieNode* node, const char *key)
{
    TrieNode *result = trienode_search(node, key);
    if (!result || !result->value)
        return NULL;
    return result;
}

static void
trienode_traverse(TrieNode* node, TRAVERSE_FUNC func)
{
    if (node) {
        if (node->value) {
            func(node);
        }
        for (TrieNode *p = node->child; p != NULL; p = p->next) {
            trienode_traverse(p, func);
        }
    }
}

static void
trienode_traverse_with_key(TrieNode *node, const char *key, TRAVERSE_WITH_KEY_FUNC func)
{
    if (node) {
        const size_t key_len = strlen(key);
        char *tmp_key = ALLOC_N(char, key_len + 2);
        strcpy(tmp_key, key);
        tmp_key[key_len] = node->c;
        tmp_key[key_len+1] = '\0';
        if (node->value) {
            func(node, tmp_key);
        }
        for (TrieNode *p = node->child; p != NULL; p = p->next) {
            trienode_traverse_with_key(p, tmp_key, func);
        }
        ruby_xfree(tmp_key);
    }
}

static unsigned long
trienode_count(TrieNode *node)
{
    unsigned long count = 0;
    if (node->value) {
        ++count;
    }
    for (TrieNode *p = node->child; p != NULL; p = p->next) {
        count += trienode_count(p);
    }
    return count;
}

/***** Helper function *****/
static VALUE
trie_traverse_value_func(const TrieNode *node)
{
    return rb_yield(node->value);
}

static VALUE
trie_traverse_key_func(const TrieNode *node, const char* key)
{
    return rb_yield(rb_str_new2(key));
}

static VALUE
trie_traverse_value_with_key_func(const TrieNode *node, const char* key)
{
    return rb_yield(rb_ary_new3(2, rb_str_new2(key), node->value));
}

/***** Trie Operation *****/
static Trie *
trie_alloc(void)
{
    Trie *trie = ALLOC(Trie);
    return trie;
}

static void
trie_free(Trie *trie)
{
    if (trie && trie->root) {
        trienode_free(trie->root);
    }
}

static void
trie_store(Trie *trie, const char *key, VALUE value)
{
    if (IS_MODIFIABLE(trie)) {
        TrieNode *node = trienode_create(trie->root, key);
        node->value = value;
    }
}

static TrieNode *
trie_search(Trie *trie, const char *key)
{
    return trienode_search(trie->root, key);
}

static TrieNode *
trie_search_leaf(Trie *trie, const char *key)
{
    return trienode_search_leaf(trie->root, key);
}

static void
trie_traverse_with_key(Trie *trie, char *key, TRAVERSE_WITH_KEY_FUNC func)
{
    ++trie->traversing;
    trienode_traverse_with_key(trie->root, key, func);
    --trie->traversing;
}

static void
trie_traverse(Trie *trie, TRAVERSE_FUNC func)
{
    ++trie->traversing;
    trienode_traverse(trie->root, func);
    --trie->traversing;
}

static void
trie_common_prefix_traverse(Trie *trie, char *prefix, TRAVERSE_WITH_KEY_FUNC func)
{
    char* sub_prefix;
    size_t len;
    TrieNode *sub = trie_search(trie, prefix);
    if (!sub) {
        return;
    }
    len = strlen(prefix);
    sub_prefix = ALLOC_N(char, len);
    strncpy(sub_prefix, prefix, len - 1);
    trienode_traverse_with_key(sub, sub_prefix, func);
    ruby_xfree(sub_prefix);
}

static unsigned long
trie_count(Trie *trie)
{
    return trienode_count(trie->root);
}

/***** Ruby Method Implementation *****/
/*** Allocation function ***/
static VALUE
rb_trie_alloc(VALUE self)
{
    return Data_Wrap_Struct(self, 0, trie_free, trie_alloc());
}

/***
 * Trie#initialize
 *
 * Initializes a new instance of Trie class.
***/
static VALUE
rb_trie_initialize(VALUE self)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    trie->root = trienode_new();
    trie->traversing = 0;
    return Qnil;
}

/***
 * Trie#store(key, value)
 * Associates the *value* with the *key*. *key* must be a String.
 * Returns *value*.
***/
static VALUE
rb_trie_store(VALUE self, VALUE key, VALUE value)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    trie_store(trie, StringValuePtr(key), value);
    return value;
}

/***
 * Trie#get(key)
 * Gets the value associated with *key*.
 * Returns the value associated with *key* if found; otherwise, nil.
***/
static VALUE
rb_trie_get(VALUE self, VALUE key)
{
    Trie *trie;
    TrieNode *result;
    Data_Get_Struct(self, Trie, trie);
    result = trie_search_leaf(trie, StringValuePtr(key));
    if (!result)
        return Qnil;
    return result->value;
}

/***
 * Trie#delete(key) -> object | nil
 * Trie#delete(key) {|key| ... } -> object
 * Deletes an element associated with the given key.
 * Returns a value of the element associated with the given key if the element is found;
 * otherwise, returns nil, but if a block is given, returns a return value of the block instead.
***/
static VALUE
rb_trie_delete(VALUE self, VALUE key)
{
    Trie *trie;
    TrieNode *result;
    VALUE val;
    Data_Get_Struct(self, Trie, trie);
    result = trie_search_leaf(trie, StringValuePtr(key));
    if (!result) {
        if (rb_block_given_p()) {
            return rb_yield(key);
        }
        return Qnil;
    }
    val = result->value;
    result->value = 0;
    return val;
}

/***
 * Trie#each { |value| ... }
 * Evaluates the given block with the values in this tree.
 * The order of evaluation is dictionary order.
 * The block must be given.
***/
static VALUE
rb_trie_each(VALUE self)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    trie_traverse_with_key(trie, "", trie_traverse_value_with_key_func);
    return self;
}

static VALUE
rb_trie_each_key(VALUE self)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    trie_traverse_with_key(trie, "", trie_traverse_key_func);
    return self;
}

static VALUE
rb_trie_each_value(VALUE self)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    trie_traverse(trie, trie_traverse_value_func);
    return self;
}

/***
 * Trie#common_prefix_each { |value| ... }
 * Evaluates the given block with the values that has the common prefix in this tree.
 * The order of evaluation is dictionary order.
 * The block must be given.
***/
static VALUE
rb_trie_common_prefix_each(VALUE self, VALUE key)
{
    Trie *trie;
    Data_Get_Struct(self, Trie, trie);
    trie_common_prefix_traverse(trie, StringValuePtr(key), trie_traverse_value_with_key_func);
    return self;
}

/***
 * Trie#size
 * Trie#length
 * Get the number of the elements in this tree.
***/
static VALUE
rb_trie_size(VALUE self)
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
    rb_define_alloc_func(cTrie, rb_trie_alloc);
    rb_define_private_method(cTrie, "initialize", rb_trie_initialize, 0);
    rb_define_method(cTrie, "store", rb_trie_store, 2);
    rb_define_method(cTrie, "[]=", rb_trie_store, 2);
    rb_define_method(cTrie, "get", rb_trie_get, 1);
    rb_define_method(cTrie, "[]", rb_trie_get, 1);
    rb_define_method(cTrie, "delete", rb_trie_delete, 1);
    rb_define_method(cTrie, "each", rb_trie_each, 0);
    rb_define_method(cTrie, "each_key", rb_trie_each_key, 0);
    rb_define_method(cTrie, "each_value", rb_trie_each_value, 0);
    rb_define_method(cTrie, "common_prefix_each", rb_trie_common_prefix_each, 1);
    rb_define_method(cTrie, "size", rb_trie_size, 0);
    rb_define_method(cTrie, "length", rb_trie_size, 0);
}

// vim: set expandtab ts=4 sw=4 sts=4:
