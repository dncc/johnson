module Johnson
  module Nodes
    SINGLE_NODES = %w{
      BitwiseNot
      Delete
      False
      Name
      Not
      Null
      Number
      Parenthesis
      PostfixIncrement
      PrefixIncrement
      PostfixDecrement
      PrefixDecrement
      Regexp
      String
      This
      Throw
      True
      Typeof
      UnaryNegative
      UnaryPositive
      Void
    }
    class Node
      include Johnson::Visitable
      include Johnson::Visitors

      attr_accessor :value
      attr_reader :line, :column
      def initialize(line, column, value)
        @line = line
        @column = column
        @value = value
      end

      def to_sexp
        SexpVisitor.new.accept(self)
      end
    end
    SINGLE_NODES.each { |se| const_set(se.to_sym, Class.new(Node)) }
  end
end
