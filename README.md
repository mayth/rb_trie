rb_trie
=======

Trie tree Implementation for Ruby

Install
=======
1. `ruby extconf.h`
2. `make && make install`

Usage
=====
`require 'trie'` first.

`Trie` class has the interface like `Hash`, Ruby's built-in class. These methods are implemented:

* `#store(key, val)`
* `#get(key)`
* `#[]=` (equivalent to `store`)
* `#[]` (equivalent to `get`)
* `#each` {|val| ...}
* `#size`
* `#length` (equivalent to `size`)

In addition, following methods are implemented.

common_prefix_each
------------------
`common_prefix_each(key) {|val| ...}`

Evaluate the given block with the values that have the key with the common prefix _key_.

License
=======
This program is licensed under the MIT License. See `LICENSE`.
