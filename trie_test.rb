require 'minitest/unit'
require './trie'

MiniTest::Unit.autorun

class TestTrie < MiniTest::Unit::TestCase
  def setup
    @trie = Trie.new
  end

  def test_store
    k = 'abc'
    v = {a: 10, b: 20, c: 30}
    assert_equal v, @trie.store(k, v)
  end

  def test_store_op
    k = 'abc'
    v = {a: 10, b: 20, c: 30}
    assert_equal v, @trie[k] = v
  end

  def test_get
    k = 'abc'
    v = {a: 10, b: 20, c: 30}
    @trie.store(k, v)
    assert_equal v, @trie.get(k)
    assert_nil @trie.get('0123')
  end

  def test_get_op
    k = 'abc'
    v = {a: 10, b: 20, c: 30}
    @trie.store(k, v)
    assert_equal v, @trie[k]
    assert_nil @trie['0123']
  end

  def test_delete
    @trie['abc'] = 100
    assert_equal 1, @trie.size
    assert_equal 100, @trie.delete('abc')
    assert_equal 0, @trie.size
    assert_nil @trie['abc']
  end

  def test_size
    @trie['abc'] = 'xyz'
    @trie['mof'] = 'alpaca'
    @trie['lilium'] = 'ambient'
    assert_equal 3, @trie.size
    assert_equal 3, @trie.length
  end

  def test_freeze
    @trie['abc'] = 'xyz'
    @trie.freeze
    assert_raises RuntimeError do
      @trie['def'] = 'mof'  # because of it is frozen.
    end
  end
end

