require 'pathname'
require Pathname(__FILE__).dirname.join('spec_base') unless $root

describe TokyoTyrant::DB, "with an open database" do

  before do
    @db = TokyoTyrant::DB.new('127.0.0.1', 45000)
    @db.clear
  end

  it "should not be nil" do
    @db.should.not.be.nil
  end

  it "should close" do
    @db.close.should.be.true
    begin
      @db.close
    rescue => e
      e.message.should == 'close error: invalid operation'
    end
  end

  it "should return a server expression" do
    @db.server.should == '127.0.0.1:45000'
  end

  it "should optimize" do
    @db.optimize.should.be.true
  end

  it "should save a value" do
    @db[:salad] = 'bacon bits'
    @db[:salad].should == 'bacon bits'
  end
  
  it "should return a value" do
    @db[:salad] = 'bacon bits'
    @db[:salad].should == 'bacon bits'
  end

  it "should save multiple values" do
    h = { 'pizza' => 'old forge style',
          'sandwich' => 'peanut butter jelly',
          'yogurt' => 'greek',
          'coffee' => 'black' }
    @db.mput(h).should.be.empty
    h.each_pair do |k,v|
      @db[k].should == v
    end
  end

  it "should delete multiple values" do
    h = { 'pizza' => 'old forge style',
          'sandwich' => 'peanut butter jelly',
          'yogurt' => 'greek',
          'coffee' => 'black' }
    @db.mput(h)
    @db.outlist('coffee', 'yogurt').should.be.empty
    @db.keys.sort.should == ['pizza','sandwich']
  end

  it "should get multiple values" do
    h = { 'pizza' => 'old forge style',
          'sandwich' => 'peanut butter jelly',
          'yogurt' => 'greek',
          'coffee' => 'black' }
    @db.mput(h)
    @db.mget(h.keys).should == h
  end

  it "should out a value" do
    k = :tomato
    @db[k] = 'green'
    @db.out(k).should.be.true
    @db[k].should.be.nil
    @db.out(k).should.be.false
  end

  it "should get a value size" do
    k = :cereal
    v = 'granola'
    @db[k] = v
    @db.vsiz(k).should == v.size
  end

  it "should check a key" do
    k = :fruit
    @db[k] = 'banana'
    @db.check(k).should.be.true
    @db.out(k)
    @db.check(k).should.be.false
  end

  it "should iterate" do
    @db[:cheese] = 'gouda'
    @db[:grapes] = 'green'
    @db[:oranges] = 'florida'
    @db[:crackers] = 'triscuit'

    @db.iterinit.should.be.true
    @db.iternext.should == 'cheese'
    @db.iternext.should == 'grapes'
    @db.iternext.should == 'oranges'
    @db.iternext.should == 'crackers'
    @db.iternext.should == nil
  end

  it "should get forward matching keys" do
    @db['apples/royalgala'] = '4173'
    @db['apples/grannysmith'] = '4139'
    @db['bananas/yellow'] = '4011'
    @db['oranges/shamouti'] = '3027'
    @db['grapefruit/deepred'] = '4288'
    @db.fwmkeys('apples').sort.should == ["apples/grannysmith", "apples/royalgala"]
  end

  it "should delete forward matching keys" do
    @db['apples/royalgala'] = '4173'
    @db['apples/grannysmith'] = '4139'
    @db['bananas/yellow'] = '4011'
    @db['oranges/shamouti'] = '3027'
    @db['grapefruit/deepred'] = '4288'
    @db.delete_keys_with_prefix('apples').should == nil
    @db.fwmkeys('apples').should.be.empty
    @db.keys.sort.should == ['bananas/yellow', 'grapefruit/deepred', 'oranges/shamouti']
  end

  it "should get all keys" do
    keys = %w[appetizers entree dessert]
    values = %w[chips chicken\ caeser\ salad ice\ cream]
    keys.each_with_index do |k,i|
      @db[k] = values[i]
    end
    @db.keys.should == keys
  end

  it "should get all values" do
    keys = %w[appetizers entree dessert]
    values = %w[chips chicken\ caeser\ salad ice\ cream]
    keys.each_with_index do |k,i|
      @db[k] = values[i]
    end
    @db.values.should == values
  end

  it "should vanish all records" do
    @db['chocolate'] = 'dark'
    @db['tea'] = 'earl grey'
    @db.empty?.should.be.false
    @db.vanish.should.be.true
    @db.empty?.should.be.true
  end

  it "should count records" do
    @db['hummus'] = 'chickpeas'
    @db['falafel'] = 'chickpeas'
    @db.rnum.should == 2
  end

  it "should report db size" do
    @db['rootbeer'] = 'virgils'
    @db.db_size.should.not == 0
  end

  it "should fetch a record" do
    @db.out('beer')
    @db.fetch('beer','heineken').should == 'heineken'
    @db['beer'] = 'heineken'
    @db.fetch('beer','becks').should == 'heineken'
  end

  it "should iterate through records" do
    keys = %w[grapejuice tacoshells rice]
    values = %w[Kedem Ortega Uncle\ Ben]
    keys.each_with_index{ |k,i| @db[k] = values[i] }

    i = 0
    @db.each do |k,v|
      k.should == keys[i]
      v.should == values[i]
      i = i += 1
    end
  end

  it "should iterate through keys" do
    keys = %w[burritos fajitas tacos tostas enchiladas]
    values = Array.new(keys.size, 'yum')
    keys.each_with_index{ |k,i| @db[k] = values[i] }

    i = 0
    @db.each_key do |k|
      k.should == keys[i]
      i = i += 1
    end
  end

  it "should iterate through values" do
    keys = %w[falafel humus couscous tabbouleh tzatziki]
    values = %w[chickpeas chickpeas semolina bulgar yogurt]
    keys.each_with_index{ |k,i| @db[k] = values[i] }

    i = 0
    @db.each_value do |v|
      v.should == values[i]
      i = i += 1
    end
  end

  it "should add serialized integer values" do
    key = 'counter'
    @db.out(key)
    @db.add_int(key, 1).should == 1
    @db.add_int(key, 1).should == 2
    @db.get_int(key).should == 2
  end

  it "should add serialized double values" do
    key = 'counter'
    @db.out(key)
    @db.add_double(key, 1.0).should.be.close?(1.0, 0.005)
    @db.add_double(key, 1.0).should.be.close?(2.0, 0.005)
    @db.get_double(key).should.be.close?(2.0, 0.005)
  end
end