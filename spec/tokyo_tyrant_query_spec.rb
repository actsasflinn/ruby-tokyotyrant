require 'pathname'
require Pathname(__FILE__).dirname.join('spec_base') unless $root

describe TokyoTyrant::Table, "with an open database" do
  @db = TokyoTyrant::Table.new('127.0.0.1', 45001)
  @db.clear
  load('spec/plu_db.rb')
  @db.mput($codes)

  it "should get a query object" do
    @db.query.class.should == TokyoTyrant::Query
  end

  it "should get keys for search conditions" do
    q = @db.query
    q.addcond('type', :streq, 'Spinach')
    q.search.sort.should == ["3332", "34173"]
  end

  it "should get ordered keys for search conditions with order" do
    q = @db.query
    q.addcond('type', :streq, 'Spinach')
    q.order_by('variety', :strasc)
    q.search.should == ["3332", "34173"]
  end

  it "should get limited keys for search conditions with limit" do
    q = @db.query
    q.addcond('type', :streq, 'Apple')
    q.setmax(10)
    q.search.sort.should == ["3072", "3073", "3074", "3075", "3076", "3077", "3078", "3348", "3349", "4182"]
  end

  it "should get records for search conditions" do
    q = @db.query
    q.addcond('type', :streq, 'Garlic')
    res = q.get.sort{ |x,y| x[:variety] <=> y[:variety] }
    res.should == [{ :__id => "4609", :variety => "Elephant", :code => "4609", :type => "Garlic" },
                   { :__id => "3401", :variety => "One-clove types", :code => "3401", :type => "Garlic" },
                   { :__id => "3052", :variety => "String", :code => "3052", :type => "Garlic" }]
  end

  it "should remove records for conditions" do
    @db.query.condition(:type, :streq, 'Orange').search.size.should == 11
    @db.query.condition(:type, :streq, 'Orange').searchout.should.be.true
    @db.query.condition(:type, :streq, 'Orange').search.size.should == 0
  end
end
