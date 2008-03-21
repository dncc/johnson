require File.expand_path(File.join(File.dirname(__FILE__), "/../../helper"))

class BinaryNodeTest < Johnson::NodeTestCase
  {
    :op_equal           => '=',
    :op_multiply_equal  => '*=',
    :op_divide_equal    => '/=',
    :op_add_equal       => '+=',
    :op_subtract_equal  => '-=',
    :op_lshift_equal    => '<<=',
    :op_rshift_equal    => '>>=',
    :op_urshift_equal   => '>>>=',
    :op_bitand_equal    => '&=',
    :op_bitxor_equal    => '^=',
    :op_bitor_equal     => '|=',
    :op_mod_equal       => '%=',
    :op_multiply        => '*',
    :op_divide          => '/',
    :op_add             => '+',
    :op_subtract        => '-',
    :op_mod             => '%',
    :op_bitand          => '&',
    :op_lshift          => '<<',
    :op_rshift          => '>>',
    :op_urshift         => '>>>',
    :op_bitxor          => '^',
    :op_bitor           => '|',
  }.each do |op,sym|
    define_method(:"test_#{op}_to_sexp") do
      assert_sexp(
                  [[op, [:name, 'i'], [:lit, 10]]],
                  @parser.parse("i #{sym} 10")
                 )
    end
  end
end
