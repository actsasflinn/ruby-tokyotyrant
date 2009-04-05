require 'pathname'
require Pathname(__FILE__).dirname.join('spec_base') unless $root

describe TokyoTyrant::Table, "with an open database" do

  before do
    @db = TokyoTyrant::Table.new('127.0.0.1', 45001)
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
  
  it "should save a value" do
    value = { :lettuce => 'Red Leaf', :dressing => 'ranch', :extra => 'bacon bits' }
    @db[:salad] = value
    @db[:salad].should == value
  end
  
  it "should return a value" do
    value = { :lettuce => 'Red Leaf', :dressing => 'ranch', :extra => 'bacon bits' }
    @db[:salad] = value
    @db[:salad].should == value
  end

  it "should save multiple values" do
    h = { 'pizza' => { :best => 'old forge style', :ok => 'new york style', :worst => 'chicago style' },
          'sandwich' => { :best => 'peanut butter jelly', :ok => 'turkey', :worst => 'olive loaf' },
          'yogurt' => { :best => 'greek', :ok => 'organic', :worst => '+hfcs' },
          'coffee' => { :best => 'black', :ok => 'espresso', :worst => 'latte' } }

    @db.mput(h).should.be.empty
    h.each_pair do |k,v|
      @db[k].should == v
    end
  end

  it "should get multiple values" do
    h = { 'pizza' => { :best => 'old forge style', :ok => 'new york style', :worst => 'chicago style' },
          'sandwich' => { :best => 'peanut butter jelly', :ok => 'turkey', :worst => 'olive loaf' },
          'yogurt' => { :best => 'greek', :ok => 'organic', :worst => '+hfcs' },
          'coffee' => { :best => 'black', :ok => 'espresso', :worst => 'latte' } }

    @db.mput(h)
    @db.mget(h.keys).should == h
  end

  it "should out a value" do
    k = :tomato
    @db[k] = { :color => 'red', :variety => 'beefy' }
    @db.out(k).should.be.true
    @db[k].should.be.nil
    @db.out(k).should.be.false
  end

  it "should check a key" do
    k = :fruit
    @db[k] = { :name => 'banana', :code => '4011' }
    @db.check(k).should.be.true
    @db.out(k)
    @db.check(k).should.be.false
  end

  it "should iterate" do
    @db[:cheese] = { :melty => 'gouda', :sharp => 'cheddar', :stringy => 'mozerella' }
    @db[:grapes] = { :sour => 'green', :big => 'red', :wine => 'purple' }
    @db[:oranges] = { :juicy => 'florida', :yellow => 'california' }
    @db[:crackers] = { :wheat => 'triscuit', :snack => 'ritz', :soup => 'saltine' }

    @db.iterinit.should.be.true
    @db.iternext.should == 'cheese'
    @db.iternext.should == 'grapes'
    @db.iternext.should == 'oranges'
    @db.iternext.should == 'crackers'
    @db.iternext.should == nil
  end

  it "should get forward matching keys" do
    @db['apples/royalgala'] = { :code => '4173', :color => 'red-yellow' }
    @db['apples/grannysmith'] = { :code => '4139', :color => 'green' }
    @db['bananas/yellow'] = { :code => '4011', :color => 'yellow' }
    @db['oranges/shamouti'] = { :code => '3027', :color => 'orange' }
    @db['grapefruit/deepred'] = { :code => '4288', :color => 'yellow/pink' }
    @db.fwmkeys('apples').sort.should == ["apples/grannysmith", "apples/royalgala"]
  end

  it "should get all keys" do
    keys = %w[appetizers entree dessert]
    values = [{ :cheap => 'chips', :expensive => 'sample everything platter' },
              { :cheap => 'hoagie', :expensive => 'steak' },
              { :cheap => 'water ice', :expensive => 'cheesecake' }]

    keys.each_with_index do |k,i|
      @db[k] = values[i]
    end
    @db.keys.should == keys
  end

  it "should get all values" do
    keys = %w[appetizers entree dessert]
    values = [{ :cheap => 'chips', :expensive => 'sample everything platter' },
              { :cheap => 'hoagie', :expensive => 'steak' },
              { :cheap => 'water ice', :expensive => 'cheesecake' }]

    keys.each_with_index do |k,i|
      @db[k] = values[i]
    end
    @db.values.should == values
  end

  it "should vanish all records" do
    @db['chocolate'] = { :type => 'dark' }
    @db['tea'] = { :type => 'earl grey' }
    @db.empty?.should.be.false
    @db.vanish.should.be.true
    @db.empty?.should.be.true
  end

  it "should count records" do
    @db['hummus'] = { :ingredients => 'chickpeas,garlic' }
    @db['falafel'] = { :ingredients => 'chickpeas,herbs' }
    @db.rnum.should == 2
  end

  it "should report db size" do
    @db['rootbeer'] = { :gourmet => 'Virgils', :natural => 'Hansens' }
    @db.size.should == 528768
  end

  it "should fetch a record" do
    @db.out('beer')
    @db.fetch('beer', { :import => 'heineken' }).should == { :import => 'heineken' }
    @db['beer'] = { :import => 'heineken' }
    @db.fetch('beer', { :import => 'becks' }).should == { :import => 'heineken' }
  end

  it "should iterate through records" do
    keys = %w[grapejuice tacoshells rice]
    values = [{ :purple => 'Kedem', :green => 'Juicy Juice' },
              { :crunchy => 'Ortega', :soft => 'Taco Bell' },
              { :brown => 'Instant', :white => 'Uncle Ben' }]
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
    values = Array.new(keys.size, { :tasty => 'yes' })
    keys.each_with_index{ |k,i| @db[k] = values[i] }

    i = 0
    @db.each_key do |k|
      k.should == keys[i]
      i = i += 1
    end
  end

  it "should iterate through values" do
    keys = %w[falafel humus couscous tabbouleh tzatziki]
    values = [{ :ingredient => 'chickpeas' },
              { :ingredient => 'chickpeas' },
              { :ingredient => 'semolina' },
              { :ingredient => 'bulgar' },
              { :ingredient => 'yogurt' }]

    keys.each_with_index{ |k,i| @db[k] = values[i] }

    i = 0
    @db.each_value do |v|
      v.should == values[i]
      i = i += 1
    end
  end
end
