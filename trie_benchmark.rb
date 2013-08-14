require 'benchmark'
require './trie'

n = 1000000
Benchmark.bm(10) do |x|
  h = {}
  t = Trie.new

  x.report('Hash []=:') do
    n.times do |i|
      h[i.to_s] = i
    end
  end
  x.report('Trie []=:') do
    n.times do |i|
      t[i.to_s] = i
    end
  end
  x.report('Hash []:') do
    n.times do |i|
      h[i.to_s]
    end
  end
  x.report('Trie []:') do
    n.times do |i|
      t[i.to_s]
    end
  end
end
