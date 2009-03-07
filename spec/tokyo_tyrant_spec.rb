require 'pathname'
require Pathname(__FILE__).dirname.join('spec_base') unless $root

describe TokyoTyrant, "with an open database" do

  before do
    @db = TokyoTyrant::DB.new('127.0.0.1', 1978)
  end

  it "should not be nil" do
    @db.should.not.be.nil
  end
  
  it "should save a value" do
    @db['salad'] = 'bacon bits'
    @db['salad'].should == 'bacon bits'
  end
  
  it "should return a value" do
    @db['salad'].should == 'bacon bits'
  end
  
end
