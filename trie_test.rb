require 'trie'

t = Trie.new

t.store('abc', {a: 10, b: 20, c: 30})
p t.get('abc')
p t.get('abcd')
t.store('xyz', 'mof')
t['abcd'] = 'alpaca'
p t.get('xyz')
p t['abcd']
puts "num of items: #{t.size}"
puts '** each'
t.each do |val|
  puts val
end

puts '** each with index'
t.each_with_index do |val, idx|
  puts "#{idx}: #{val}"
end

puts '**common prefix: abc'
t.common_prefix_each('abc') do |val|
  puts val
end

