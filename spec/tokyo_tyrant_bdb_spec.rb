require 'pathname'
require Pathname(__FILE__).dirname.join('spec_base') unless $root

describe TokyoTyrant::BDB, "with an open database" do
  before do
    @db = TokyoTyrant::BDB.new('127.0.0.1', 45003)
    @db.clear
  end

  it "should put duplicate keys and values" do
    key = 'desert'
    values = ['baklava', 'chocolate mousse', 'tiramisu', 'tiramisu']
    h = { key => values }
    @db.outlist(key)
    values.each do |value|
      @db.putdup(key, value).should.be.true
    end
    @db.getlist(h.keys).should == h
  end

  it "should put a list" do
    h = { 'mushrooms' => ['portobello', 'button'] }
    @db.outlist(h.keys)
    @db.putlist(h).should == []
    @db.getlist(h.keys).should == h
  end

  it "should getlist" do
    h = { 'mushrooms' => ['portobello', 'button'] }
    @db.outlist(h.keys)
    @db.putlist(h)
    @db.getlist(h.keys).should == h
  end
end