//
// Created by mzeman on 12.01.22.
//

#ifndef FJP_SP_VALUE_H
#define FJP_SP_VALUE_H


#include <string>
#include <map>
#include "synt_tree.h"

struct method_call;
struct boolean_expression;
struct variable_ref;
struct arithmetic;
struct assign_expression;

// Every possible value structure
struct value{
    enum class value_type {
        const_number_lit,
        const_string_lit,
        variable,
        return_value,
        member,
        array_member,
        arithmetic,
        ternary,
        assign_expression,
        boolean_expression,
    };

    value_type value_type;
    union {
        int number_value;
        variable_ref* variable;
        method_call* call;
        struct {
            variable_ref* array;
            value* index;
        } array_element;
        arithmetic* arithmetic_expression;
        struct {
            boolean_expression* boolexpr;
            value* positive;
            value* negative;
        } ternary;
        assign_expression* assign_expr;
        boolean_expression* boolexpr;
    } content;

    std::string member_spec;
    std::string name; //struct_name
    bool return_value_ignored = false;

    value(int val) : value_type(value_type::const_number_lit) {
        content.number_value = val;
    }

    value(const std::string& val) : value_type(value_type::const_string_lit) {

        // removing quote marks
        if (val.size() >= 2 && val[0] == '\"' && val[val.size() - 1] == '\"') {
            name = val.substr(1, val.size() - 2);
        }
        else {
            name = val;
        }
    }

    value(boolean_expression* exp, value* positive, value* negative) : value_type(value_type::ternary) {
        content.ternary.boolexpr = exp;
        content.ternary.positive = positive;
        content.ternary.negative = negative;
    }

    value(arithmetic* arithmetic_expr) : value_type(value_type::arithmetic) {
        content.arithmetic_expression = arithmetic_expr;
    }

    value(assign_expression* assign_expr) : value_type(value_type::assign_expression) {
        content.assign_expr = assign_expr;
    }

    value(variable_ref* varref) : value_type(value_type::variable) {
        content.variable = varref;
    }

    value(const std::string& structname, const std::string& member) : value_type(value_type::member) {
        name = structname;
        member_spec = member;
    }

    value(method_call* func_call) : value_type(value_type::return_value) {
        content.call = func_call;
    }

    value(variable_ref* array, value* index) : value_type(value_type::array_member) {
        content.array_element.array = array;
        content.array_element.index = index;
    }

    value(boolean_expression* bool_expr) : value_type(value_type::boolean_expression) {
        content.boolexpr = bool_expr;
    }

    virtual ~value();

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers);

    pl0_utils::pl0type_info get_type_info() const;
};



// bool expressions as value
struct boolean_expression {

    enum class operation {
        none,
        b_and,  // &&
        b_or,   // ||
        c_gt,   // >
        c_lt,   // <
        c_ge,   // >=
        c_le,   // <=
        negate, // !
        c_eq,   // ==
        c_neq,  // !=
    };

    // get boolean by its string value
    static operation convert_to_bool_operation(const std::string& op) {
        if (op == "||") {
            return operation::b_or;
        }
        else if (op == "==") {
            return operation::c_eq;
        }
        else if (op == "!=") {
            return operation::c_neq;
        }
        else if (op == "&&") {
            return operation::b_and;
        }
        else if (op == ">=") {
            return operation::c_ge;
        }
        else if (op == "<=") {
            return operation::c_le;
        }
        else if (op == ">") {
            return operation::c_gt;
        }
        else if (op == "<") {
            return operation::c_lt;
        }
        return operation::none;
    }

    // converts operation to pl0-code OPR argument
    static pl0_utils::pl0code_opr convert_to_pl0(const operation operation) {
        switch (operation) {
            case operation::c_eq: return pl0_utils::pl0code_opr::EQUAL;
            case operation::c_ge: return pl0_utils::pl0code_opr::GREATER_OR_EQUAL;
            case operation::c_le: return pl0_utils::pl0code_opr::LESS_OR_EQUAL;
            case operation::c_neq: return pl0_utils::pl0code_opr::NOTEQUAL;
            case operation::c_gt: return pl0_utils::pl0code_opr::GREATER;
            case operation::c_lt: return pl0_utils::pl0code_opr::LESS;
            default: return pl0_utils::pl0code_opr::NEGATE;
        }
    }

    boolean_expression(value* val, value* val2, operation comp_op = operation::none)
            : cmpval1(val), cmpval2(val2), op(comp_op) {
    }

    boolean_expression(boolean_expression* bval, boolean_expression* bval2, operation bool_op = operation::none)
            : boolexp1(bval), boolexp2(bval2), op(bool_op) {
    }

    boolean_expression(bool value)
            : preset_value(value) {
    }

    virtual ~boolean_expression();


    // operation to be performed
    operation op = operation::none;
    // if this is a constant literal, this stores its value
    bool preset_value = false;
    // values to be compared
    value* cmpval1 = nullptr, *cmpval2 = nullptr;
    // boolean expression(s) to be considered
    boolean_expression* boolexp1 = nullptr, *boolexp2 = nullptr;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions);
};



// calling of the method command
struct method_call{
    method_call(char* identifier, std::list<value*>* parameters_list = nullptr)
            : function_identifier(identifier), parameters(parameters_list) {
    }
    virtual ~method_call();

    // calling by method identifier
    std::string function_identifier;
    // method parameters
    std::list<value*>* parameters;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions);
};


// wrapper for variable reference in value proj
struct variable_ref {
    variable_ref(const char* identifier) : identifier(identifier) {
    }

    // variable identifier
    std::string identifier;

    generation_result generate(std::map<std::string, declared_identifier> declared_identifiers) const{

        // check if identifier exists
        if (declared_identifiers.find(identifier) == declared_identifiers.end()) {
            return generate_result(evaluate_error::undeclared_identifier,
                                   "Undeclared identifier '" + identifier + "'");
        }

        return generate_result(evaluate_error::ok, "ok");
    }
};


// arithmetic expression
struct arithmetic {
    enum class operation {
        add,    // +
        sub,    // -
        mul,    // *
        div,    // /
        none,
    };

    // converts string to enum fo certain operation
    static operation get_operation(const std::string& str) {
        if (str == "*") {
            return operation::mul;
        }
        else if (str == "/") {
            return operation::div;
        }
        else if (str == "+") {
            return operation::add;
        }
        else if (str == "-") {
            return operation::sub;
        }
        return operation::none;
    }

    // converts operation to p-code OPR argument
    static pl0_utils::pl0code_opr convert_to_pl0(operation operation) {
        switch (operation) {
            case operation::mul: return pl0_utils::pl0code_opr::MULTIPLY;
            case operation::div: return pl0_utils::pl0code_opr::DIVIDE;
            case operation::add: return pl0_utils::pl0code_opr::ADD;
            case operation::sub: return pl0_utils::pl0code_opr::SUBTRACT;
            default: return pl0_utils::pl0code_opr::NEGATE;
        }
    }

    arithmetic(value* lhs, operation valop, value* rhs)
            : lhs_val(lhs), rhs_val(rhs), op(valop) {
    }
    virtual ~arithmetic();

    // left- and right-hand side of arithmetic operation
    value* lhs_val, *rhs_val;
    // operation to be performed
    operation op;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions);
};

// wrapper of value that helps to discard stack record
struct expression {

    expression() {}

    expression(value* val)
            : evaluate_value(val) {
    }

    virtual ~expression();

    value* evaluate_value = nullptr;

    virtual generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions);
};


// value assignment to variable
struct assign_expression : public expression {

    //constructor for value/array assignment
    assign_expression(char* identifier, value* array_index, value* val)
            : identifier(identifier), array_index(array_index), assign_value(val) {
    }

    //constructor for struct assignment
    assign_expression(char* struct_identifier, char* member_identifier, value* val)
            : identifier(struct_identifier), struct_member_identifier(member_identifier),
                assign_value(val), struct_member(true) {
    }

    virtual ~assign_expression();

    std::string struct_member_identifier;
    bool struct_member = false;
    value* array_index = nullptr;
    std::string identifier;
    value* assign_value;

    //if this is a right-hand side of another assignment, push the resulting left-hand side to stack
    bool push_result_to_stack = false;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions) override;
};



#endif //FJP_SP_VALUE_H
