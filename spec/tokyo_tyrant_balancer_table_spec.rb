require 'pathname'
require Pathname(__FILE__).dirname.join('spec_base') unless $root

describe TokyoTyrant::Balancer::Table, "with an open database" do

  before do
    @db = TokyoTyrant::Balancer::Table.new(['127.0.0.1:45001'])
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

  it "should optimize" do
    @db.optimize.should.be.true
  end

  it "should save a value" do
    value = { 'lettuce' => 'Red Leaf', 'dressing' => 'ranch', 'extra' => 'bacon bits' }
    @db['salad'] = value
    @db['salad'].should == value
  end
  
  it "should return a value" do
    value = { 'lettuce' => 'Red Leaf', 'dressing' => 'ranch', 'extra' => 'bacon bits' }
    @db['salad'] = value
    @db['salad'].should == value
  end

  it "should accept binary data" do
    s = "mango#{0.chr}salsa"
    h = { s => s }
    @db.put(s, h).should.be.true
    @db[s].should.equal(h)
  end

  it "should out a value" do
    k = 'tomato'
    @db[k] = { 'color' => 'red', 'variety' => 'beefy' }
    @db.out(k).should.be.true
    @db[k].should.be.nil
    @db.out(k).should.be.false
  end

  it "should check a key" do
    k = 'fruit'
    @db[k] = { 'name' => 'banana', 'code' => '4011' }
    @db.check(k).should.be.true
    @db.out(k)
    @db.check(k).should.be.false
  end

  it "should get forward matching keys" do
    @db['apples/royalgala'] = { 'code' => '4173', 'color' => 'red-yellow' }
    @db['apples/grannysmith'] = { 'code' => '4139', 'color' => 'green' }
    @db['bananas/yellow'] = { 'code' => '4011', 'color' => 'yellow' }
    @db['oranges/shamouti'] = { 'code' => '3027', 'color' => 'orange' }
    @db['grapefruit/deepred'] = { 'code' => '4288', 'color' => 'yellow/pink' }
    @db.fwmkeys('apples').sort.should == ["apples/grannysmith", "apples/royalgala"]
  end

  it "should delete forward matching keys" do
    @db['apples/royalgala'] = { 'code' => '4173', 'color' => 'red-yellow' }
    @db['apples/grannysmith'] = { 'code' => '4139', 'color' => 'green' }
    @db['bananas/yellow'] = { 'code' => '4011', 'color' => 'yellow' }
    @db['oranges/shamouti'] = { 'code' => '3027', 'color' => 'orange' }
    @db['grapefruit/deepred'] = { 'code' => '4288', 'color' => 'yellow/pink' }
    @db.delete_keys_with_prefix('apples').should == nil
    @db.fwmkeys('apples').should.be.empty
    @db.keys.sort.should == ['bananas/yellow', 'grapefruit/deepred', 'oranges/shamouti']
  end

  it "should get all keys" do
    keys = %w[appetizers entree dessert]
    values = [{ 'cheap' => 'chips', 'expensive' => 'sample everything platter' },
              { 'cheap' => 'hoagie', 'expensive' => 'steak' },
              { 'cheap' => 'water ice', 'expensive' => 'cheesecake' }]

    keys.each_with_index do |k,i|
      @db[k] = values[i]
    end
    @db.keys.should == keys
  end

  it "should get all values" do
    keys = %w[appetizers entree dessert]
    values = [{ 'cheap' => 'chips', 'expensive' => 'sample everything platter' },
              { 'cheap' => 'hoagie', 'expensive' => 'steak' },
              { 'cheap' => 'water ice', 'expensive' => 'cheesecake' }]

    keys.each_with_index do |k,i|
      @db[k] = values[i]
    end
    @db.values.should == values
  end

  it "should vanish all records" do
    @db['chocolate'] = { 'type' => 'dark' }
    @db['tea'] = { 'type' => 'earl grey' }
    @db.empty?.should.be.false
    @db.vanish.should.be.true
    @db.empty?.should.be.true
  end

  it "should count records" do
    @db['hummus'] = { 'ingredients' => 'chickpeas,garlic' }
    @db['falafel'] = { 'ingredients' => 'chickpeas,herbs' }
    @db.rnum.should == 2
  end

  it "should fetch a record" do
    @db.out('beer')
    @db.fetch('beer'){{ 'import' => 'heineken' }}.should == { 'import' => 'heineken' }
    @db['beer'].should == { 'import' => 'heineken' }
    @db.fetch('beer'){{ 'import' => 'becks' }}.should == { 'import' => 'heineken' }
  end

  it "should add serialized integer values" do
    key = 'counter'
    @db.out(key)
    @db[key] = { 'title' => 'Bean Counter' }
    @db.add_int(key, 1).should == 1
    @db.add_int(key, 1).should == 2
    @db.get_int(key).should == 2
    @db[key].should == { 'title' => 'Bean Counter', '_num' => "2" }
  end

  it "should increment integer values" do
    key = 'counter'
    @db.out(key)
    @db[key] = { 'title' => 'Bean Counter' }
    @db.increment(key).should == 1
    @db.increment(key, 2).should == 3
    @db.get_int(key).should == 3
    @db[key].should == { 'title' => 'Bean Counter', '_num' => "3" }
  end

  it "should add serialized double values" do
    key = 'counter'
    @db.out(key)
    @db[key] = { 'title' => 'Bean Counter' }
    @db.add_double(key, 1.0).should.be.close?(1.0, 0.005)
    @db.add_double(key, 1.0).should.be.close?(2.0, 0.005)
    @db.get_double(key).should.be.close?(2.0, 0.005)
    @db[key].should == { 'title' => 'Bean Counter', '_num' => "2" }
  end

  it "should set an index" do
    key = 'counter'
    50.times do |i|
      @db["key#{i}"] = { 'title' => %w{rice beans corn}.sort_by{rand}.first,
                         'description' => 'a whole protein' }
    end
    @db.set_index('title', :lexical).should.be.true
  end

  it "should serialize objects that respond to to_tokyo_tyrant" do
    class Thing
      def to_tokyo_tyrant
        "success"
      end
    end

    @db["to_tokyo_tyrant"] = { "thing" => Thing.new }
    @db["to_tokyo_tyrant"].should == { "thing" => "success" }
  end
end
