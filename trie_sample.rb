require './trie'

trie = Trie.new
trie['adc'] = 100
trie['abc'] = 200
trie['xyz'] = 300
trie['def'] = 400
trie['abcd'] = 500
trie['abcee'] = 600
puts '--- each'
trie.each {|k, v| puts "#{k}: #{v}"}
puts '--- each_key'
trie.each_key {|k| puts k}
puts '--- each_value'
trie.each_value {|v| puts v}
puts '--- common_prefix_each'
trie.common_prefix_each('abc') {|k, v| puts "#{k}: #{v}"}
