require 'pathname'
require Pathname(__FILE__).dirname.join('spec_base') unless $root

describe TokyoTyrant::Query, "with an open database" do
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
    q.search.sort.should == %w[3332 34173]
  end

  it "should get keys for negated search conditions" do
    q = @db.query
    q.addcond('type', '!streq', 'Spinach')
    res = q.search
    %w[3332 34173].each do |k|
      res.should.not.include?(k)
    end
  end

  it "should get ordered keys for search conditions with default order" do
    q = @db.query
    q.addcond('type', :streq, 'Spinach')
    q.setorder('variety')
    q.search.should == ["3332", "34173"]
  end

  it "should get ordered keys for search conditions with ascending order" do
    q = @db.query
    q.addcond('type', :streq, 'Spinach')
    q.setorder('variety', :strasc)
    q.search.should == ["3332", "34173"]
  end

  it "should get ordered keys for search conditions with decending order" do
    q = @db.query
    q.addcond('type', :streq, 'Spinach')
    q.order_by('variety', :StrDesc)
    q.search.should == ["34173", "3332"]
  end

  it "should get limited keys for search conditions with limit" do
    q = @db.query{ |q| 
      q.addcond('type', :streq, 'Apple')
      q.order_by('variety', :strdesc)
      q.setlimit(10)
    }.should == ["4860", "3011", "3271", "3382", "4182", "3353", "4176", "3272", "3297", "3009"]
  end

  it "should get limited keys for search conditions with limit and offset" do
    q = @db.query{ |q| 
      q.addcond('type', :streq, 'Apple')
      q.order_by('variety', :strdesc)
      q.setlimit(10, 10)
    }.should == ["3008", "3352", "3077", "3349", "3076", "3073", "3348", "3007", "3078", "3347"]
  end

  it "should get records for search conditions" do
    q = @db.query
    q.addcond('type', :streq, 'Garlic')
    res = q.get.sort{ |x,y| x['variety'] <=> y['variety'] }
    res.should == [{ '__id' => "4609", 'variety' => "Elephant", 'code' => "4609", 'type' => "Garlic" },
                   { '__id' => "3401", 'variety' => "One-clove types", 'code' => "3401", 'type' => "Garlic" },
                   { '__id' => "3052", 'variety' => "String", 'code' => "3052", 'type' => "Garlic" }]
  end

  it "should remove records for conditions" do
    q = @db.query
    q.addcond('type', :streq, 'Orange')
    q.search.size.should == 11
    q.searchout.should.be.true
    q.search.size.should == 0
  end

  it "should chain search options" do
    res = @db.query.condition('type', :streq, 'Cucumber').order_by('variety', :strdesc).limit(3).search
    res.should == ["4596", "4595", "4594"]
  end

  it "should query with a block" do
    res = @db.query do |q|
      q.condition('type', :streq, 'Cucumber')
      q.order_by('variety', :strdesc)
      q.limit(3)
    end
    res.should == ["4596", "4595", "4594"]
  end

  it "should find with a block" do
    res = @db.find do |q|
      q.condition('type', :streq, 'Cucumber')
      q.order_by('variety', :strdesc)
      q.limit(3)
    end
    res.should == [{'type'=>"Cucumber", 'variety'=>"Pickling / Gherkin", '__id'=>"4596", 'code'=>"4596"},
                   {'type'=>"Cucumber", 'variety'=>"Lemon", '__id'=>"4595", 'code'=>"4595"},
                   {'type'=>"Cucumber", 'variety'=>"Japanese / White", '__id'=>"4594", 'code'=>"4594"}]
  end

  it "should show query count" do
    res = @db.prepare_query{ |q| q.condition('type', :streq, 'Cucumber') }.count
    res.should == 5
  end

  it "should allow a custom pkey for a result set" do
    q = @db.query
    q.condition('type', :streq, 'Cucumber')
    q.set_pkey(:pk)
    q.get.should == [{'type'=>"Cucumber", 'code'=>"4592", :pk=>"4592", 'variety'=>"Armenian"},
                     {'type'=>"Cucumber", 'code'=>"4593", :pk=>"4593", 'variety'=>"English / Hot House / Long Seedless / Telegraph / Continental"},
                     {'type'=>"Cucumber", 'code'=>"4594", :pk=>"4594", 'variety'=>"Japanese / White"},
                     {'type'=>"Cucumber", 'code'=>"4595", :pk=>"4595", 'variety'=>"Lemon"},
                     {'type'=>"Cucumber", 'code'=>"4596", :pk=>"4596", 'variety'=>"Pickling / Gherkin"}]
  end

  it "should return a query hint" do
    @db.set_index(:type, :lexical)
    q = @db.query
    q.condition(:type, :streq, 'Cucumber')
    q.order_by(:code)
    q.limit(3)
    q.run
    q.hint.should == "\nusing an index: \"type\" asc (STREQ)\nresult set size: 5\nsorting the result set: \"code\"\n"
  end
end
