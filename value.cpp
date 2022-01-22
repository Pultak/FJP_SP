//
// Created by pultak on 13.01.22.
//
#include "value.h"

value::~value() {
    switch (value_type) {
        case value_type::const_number_lit:
        case value_type::const_string_lit:
        default:
            break;
        case value_type::variable:
            delete content.variable;
            break;
        case value_type::return_value:
            delete content.call;
            break;
        case value_type::assign_expression:
            delete content.assign_expr;
            break;
        case value_type::boolean_expression:
            delete content.boolexpr;
            break;
        case value_type::array_member:
            delete content.array_element.array;
            delete content.array_element.index;
            break;
        case value_type::arithmetic:
            delete content.arithmetic_expression;
            break;
        case value_type::ternary:
            delete content.ternary.boolexpr;
            delete content.ternary.positive;
            delete content.ternary.negative;
            break;
    }
}

pl0_utils::pl0type_info get_identifier_type_info(const std::string& identifier,
                                                 std::map<std::string, declared_identifier>& declared_identifiers){
    if (!(declared_identifiers.find(identifier) != declared_identifiers.end())) {
        return { TYPE_VOID, 0, false };
    }

    return declared_identifiers.find(identifier)->second.type;
}


pl0_utils::pl0type_info value::get_type_info(std::map<std::string, declared_identifier>& declared_identifiers,
                                             std::map<std::string, std::list<declaration*>*> struct_defs) const{
    switch (value_type) {
        case value_type::const_number_lit:
            return { TYPE_INT, 0, false };
        case value_type::const_string_lit:
            return { TYPE_STRING, 0, false };
        case value_type::variable:
            return get_identifier_type_info(content.variable->identifier, declared_identifiers);
        case value_type::array_member:
            return get_identifier_type_info(content.array_element.array->identifier, declared_identifiers);
        case value_type::assign_expression:
            return content.assign_expr->assign_value->get_type_info(declared_identifiers, struct_defs);
            break;
        case value_type::boolean_expression:
            return { TYPE_BOOL, 0, false };
        case value_type::member:
        {
            // get name based on the identifier
            if (declared_identifiers.find(name) == declared_identifiers.end()) {
                return { TYPE_VOID, 0, false };
            }
            const auto& def = declared_identifiers.find(name)->second;

            // load the struct definition
            auto struct_def = struct_defs.find(def.struct_name)->second;

            for (auto member : *struct_def) {
                if (member->identifier == member_spec) {
                    return member->type;
                }
            }

            return { TYPE_VOID, 0, false };
        }
        case value_type::arithmetic:
            return { TYPE_INT, 0, false };
        case value_type::ternary:
        {
            pl0_utils::pl0type_info positivetype = content.ternary.positive->get_type_info(declared_identifiers, struct_defs);
            pl0_utils::pl0type_info negativetype = content.ternary.negative->get_type_info(declared_identifiers, struct_defs);

            if (positivetype != negativetype) {
                return { TYPE_VOID, 0, false };
            }

            return positivetype;
        }
        case value_type::return_value:
            return get_identifier_type_info(content.call->function_identifier, declared_identifiers);
    }

    return { TYPE_VOID, 0, false };
}


generation_result value::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                  std::map<std::string, declared_identifier>& declared_identifiers,
                                  std::map<std::string, std::list<declaration*>*> struct_defs) {

    // pushing value to stack, unless "return_value_ignored" is defined

    generation_result ret = generate_result(evaluate_error::ok, "");
    int address;

    switch (value_type) {
        case value_type::const_number_lit: {
            // constant integer literal - push constant to stack
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, content.number_value);
            break;
        }
        case value_type::variable: {
            // variable - load by address
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, content.variable->identifier);
            ret = content.variable->generate(declared_identifiers);
            break;
        }
        case value_type::arithmetic: {
            // math expression - just evaluate, arithmetic_expression evaluate method will leave result on stack
            ret = content.arithmetic_expression->generate(result_instructions, declared_identifiers, struct_defs);
            break;
        }
        case value_type::member: {
            // struct member - determine struct base and member offset and load it
            // checks if identifier is declared
            if (declared_identifiers.find(name) == declared_identifiers.end()) {
                ret = generate_result(evaluate_error::undeclared_identifier, "Undeclared identifier '" + name + "'");
                break;
            }

            // get the struct name from identifier
            std::string struct_name = declared_identifiers[name].struct_name;

            // struct must be defined
            if (struct_defs.find(struct_name) == struct_defs.end()) {
                ret = generate_result(evaluate_error::unknown_typename, "Unknown typename found!");
                break;
            }

            // load the struct definition
            auto struct_def = struct_defs[struct_name];

            // compute the offset by summing sizes of struct members till we find the required member
            int size = 0;
            bool found = false;
            for (auto member: *struct_def) {
                if (member->identifier == member_spec) {
                    found = true;
                    break;
                }
                size += member->determine_size(struct_defs);
            }

            // the member was not found
            if (!found) {
                ret = generate_result(evaluate_error::undeclared_struct_member, "Undeclared struct member '" + member_spec + "'");
                break;
            }

            // new pcode_arg with the size as a offset
            auto p_arg = pl0_utils::pl0code_arg(name);
            p_arg.offset = size;

            result_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 0, p_arg);

            break;
        }
        case value_type::array_member: {
            // array element - load base, move by offset and load using extended p-code instruction set
            // specify the base
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);

            // evaluate index value
            ret = content.array_element.index->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                break;
            }

            // store the address of the start of the array
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT,
                                             pl0_utils::pl0code_arg(content.array_element.array->identifier));

            // add the index to the address of the start
            result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR,
                                             pl0_utils::pl0code_opr::ADD);

            // put the value given by the address present at the top of the stack to the top of the stack
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LDA, 0, 0);
            break;
        }
        case value_type::ternary:{
            // ternary operator - evaluate condition and push value to stack according to boolean_expression result
            // evaluate conditional
            ret = content.ternary.boolexpr->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                break;
            }
            // jump to false branch if false (address will be stored later, once known)
            int jpcpos = result_instructions.size();
            result_instructions.emplace_back(pl0_utils::pl0code_fct::JPC, 0);
            // evaluate "true" branch
            ret = content.ternary.positive->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                break;
            }
            // jump below "false" branch
            int jmppos = result_instructions.size();
            result_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, 0);
            // fill in the "false" JPC jump address

            result_instructions[jpcpos].arg.value = static_cast<int>(result_instructions.size());
            // evaluate "false" branch
            ret = content.ternary.negative->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                break;
            }
            // fill the "true" branch jump destination
            result_instructions[jmppos].arg.value = static_cast<int>(result_instructions.size());

            break;
        }
            // assign expression - set "push to stack" flag and evaluate
        case value_type::assign_expression:
            // this will cause assign expression to push left-hand side of assignment to stack after assignment
            content.assign_expr->push_result_to_stack = true;
            ret = content.assign_expr->generate(result_instructions, declared_identifiers, struct_defs);
            break;
            // boolean expression - evaluate, result is stored to stack (0 or 1)
        case value_type::boolean_expression:
            ret = content.boolexpr->generate(result_instructions, declared_identifiers, struct_defs);
            break;
        case value_type::return_value:
            // return value - evaluate the call, caller will store return value to a specific location
            ret = content.call->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                break;
            }
            // return value is at 0;ReturnValueCell (callee put it there)
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 0, ReturnValueCell);
            break;
    }

    // if evaluated ok, but outer context decided to ignore return value, move stack pointer
    if (ret.result == evaluate_error::ok && return_value_ignored) {
        if(return_value_ignored){
            result_instructions.emplace_back(pl0_utils::pl0code_fct::INT, -1);
            return generate_result(evaluate_error::ok, "");
        }else{
            return generate_result(evaluate_error::func_no_return, "Function is without return value!");
        }
    }else{
        return ret;
    }

}

boolean_expression::~boolean_expression() {
    if (cmpval1) {
        delete cmpval1;
    }
    if (cmpval2) {
        delete cmpval2;
    }
    if (boolexp1) {
        delete boolexp1;
    }
    if (boolexp2) {
        delete boolexp2;
    }
}

generation_result boolean_expression::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                               std::map<std::string, declared_identifier>& declared_identifiers,
                                               std::map<std::string, std::list<declaration*>*> struct_defs) {

    generation_result ret = generate_result(evaluate_error::ok, "");

    // if cmpval1 is defined, it's comparation
    if (cmpval1) {
        // cmpval2 defined = perform comparison of two values
        if (cmpval2) {
            // evaluate first value
            ret = cmpval1->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }
            // evaluate second value
            ret = cmpval2->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            // boolean AND - sum of last 2 values must be 2 (both are 1)
            if (op == operation::b_and) {
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::ADD);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 2);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::EQUAL);
            }
            // boolean OR - sum of last 2 values must be non-0 (at least one is 1)
            else if (op == operation::b_or) {
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::ADD);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::NOTEQUAL);
            }
                // compare them - result is 0 or 1 on stack
            else {
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, convert_to_pl0(op));
            }

            auto type1 = cmpval1->get_type_info(declared_identifiers, struct_defs);
            auto type2 = cmpval2->get_type_info(declared_identifiers, struct_defs);
            if (type1 != type2) {
                return generate_result(evaluate_error::cannot_assign_type, "Cannot compare values with different types");
            }
        }
        else if (op == operation::none) { // is cmpval1 true = is its value non-zero?
            ret = cmpval1->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);
            result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::NOTEQUAL);
        }
        else if (op == operation::negate) { // negate cmpval1 and evaluate

            ret = cmpval1->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            // negation == comparison with zero
            // 1 == 0 --> 0
            // 0 == 0 --> 1

            result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);
            result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::EQUAL);
        }
    }
        // no cmpval defined, but boolexp1 is defined - perform boolean operations
    else if (boolexp1) {
        // second boolean expression is defined, perform binary boolean operation
        if (boolexp2) {

            // evaluate first expression
            ret = boolexp1->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }
            // evaluate second expression
            ret = boolexp2->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            // p-code does not support boolean expressions, so we must use math operations
            // to compensate for their absence

            // 0/1 and 0/1 on stack - perform some operations according to given operation

            // boolean AND - sum of last 2 values must be 2 (both are 1)
            if (op == operation::b_and) {
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::ADD);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 2);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::EQUAL);
            }
                // boolean OR - sum of last 2 values must be non-0 (at least one is 1)
            else if (op == operation::b_or) {
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::ADD);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::NOTEQUAL);
            }
                // no other binary boolean operator is present
            else {
                return generate_result(evaluate_error::invalid_state, "Invalid state - symbol 'boolean_expression' could not be evaluated in given context");
            }

        }
            // just one bool expression is present - this means we either evaluate assertion or negation of it
        else {

            // negation - "!" proj present before
            if (op == operation::negate) {

                // evaluate it, result is on stack (0 or 1)
                ret = boolexp1->generate(result_instructions, declared_identifiers, struct_defs);
                if (ret.result != evaluate_error::ok) {
                    return ret;
                }

                // negation == comparison with zero
                // 1 == 0 --> 0
                // 0 == 0 --> 1

                result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);
                result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::EQUAL);
            }
                // evaluate assertion (no operation)
            else {

                // evaluate it and leave result on stack as is
                ret = boolexp1->generate(result_instructions, declared_identifiers, struct_defs);
                if (ret.result != evaluate_error::ok) {
                    return ret;
                }
            }
        }
    }
        // no value or expression node is defined, use "preset value" - this means the boolean expression consists solely of a bool literal (true or false)
    else {
        // use "preset_value"
        result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, preset_value ? 1 : 0);
    }

    return generate_result(evaluate_error::ok, "");
}

method_call::~method_call(){
    if (parameters) {
        delete parameters;
    }
}

generation_result method_call::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                        std::map<std::string, declared_identifier>& declared_identifiers,
                                        std::map<std::string, std::list<declaration*>*> struct_defs) {

    generation_result ret = generate_result(evaluate_error::ok, "");

    // check if the identifier exist
    if (declared_identifiers.find(function_identifier) == declared_identifiers.end()) {
        std::string msg = "Undeclared identifier '" + function_identifier + "'";
        return generate_result(evaluate_error::undeclared_identifier, msg);
    }

    // retrieve declaration info
    const auto& declinfo = declared_identifiers.find(function_identifier)->second;

    // identifier must represent a function
    if (!declinfo.type.is_function) {
        std::string msg = "Identifier '" + function_identifier + "' is not a function";
        return generate_result(evaluate_error::invalid_call, msg);
    }

    if (!parameters && declinfo.parameters.size() > 0) {
        std::string msg = "Wrong number of parameters for '" + function_identifier + "' function call";
        return generate_result(evaluate_error::invalid_call, msg);
    }

    // push parameters to stack, if there should be some
    // the caller then refers to every parameter as to a cell with negative position (-1 in his base is the last parameter pushed by caller, etc.)
    if (parameters) {

        if (parameters->size() != declinfo.parameters.size()) {
            std::string msg = "Wrong number of parameters for '" + function_identifier + "' function call";
            return generate_result(evaluate_error::invalid_call, msg);
        }

        size_t parampos = 0;

        for (value* param : *parameters) {

            auto pi = param->get_type_info(declared_identifiers, struct_defs);

            if (pi != declinfo.parameters[parampos]) {
                std::string msg = "Type of parameter " + std::to_string(parampos) + " for '" + function_identifier + "' function call does not match";
                return generate_result(evaluate_error::invalid_call, msg);
            }
            parampos++;

            ret = param->generate(result_instructions, declared_identifiers, struct_defs);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }
        }
    }

    // call the function
    result_instructions.emplace_back(pl0_utils::pl0code_fct::CAL, pl0_utils::pl0code_arg(function_identifier, true));

    // clean up stack after function call
    if (parameters && parameters->size() > 0) {
        result_instructions.emplace_back(pl0_utils::pl0code_fct::INT, -static_cast<int>(parameters->size()));
    }

    return generate_result(evaluate_error::ok, "");
}

math::~math() {
    delete lhs_val;
    delete rhs_val;
}

generation_result math::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                 std::map<std::string, declared_identifier>& declared_identifiers,
                                 std::map<std::string, std::list<declaration*>*> struct_defs){

    generation_result ret = generate_result(evaluate_error::ok, "");

    // all math expressions are binary in grammar

    // evaluate left-hand side
    ret = lhs_val->generate(result_instructions, declared_identifiers, struct_defs);
    if (ret.result != evaluate_error::ok) {
        return ret;
    }

    // evaluate right-hand side
    ret = rhs_val->generate(result_instructions, declared_identifiers, struct_defs);
    if (ret.result != evaluate_error::ok) {
        return ret;
    }

    // perform operation on two top values from stack
    result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, convert_to_pl0(op));

    auto type1 = lhs_val->get_type_info(declared_identifiers, struct_defs);
    auto type2 = rhs_val->get_type_info(declared_identifiers, struct_defs);
    if (type1.parent_type != TYPE_INT || type2.parent_type != TYPE_INT) {
        return generate_result(evaluate_error::cannot_assign_type, "Cannot perform math operation on non-integer values");
    }

    return generate_result(evaluate_error::ok, "");
}

expression::~expression() {
    if (evaluate_value) {
        delete evaluate_value;
    }
}

generation_result expression::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                       std::map<std::string, declared_identifier>& declared_identifiers,
                                       std::map<std::string, std::list<declaration*>*> struct_defs) {

    // wrap "value" evaluation, but ignore return value (discard stack value by calling INT 0 -1)
    if (evaluate_value) {
        evaluate_value->return_value_ignored = true;
        return evaluate_value->generate(result_instructions, declared_identifiers, struct_defs);
    }
    else {
        return generate_result(evaluate_error::invalid_state, "Invalid state - symbol 'expression' could not be evaluated in given context");
    }


}

assign_expression::~assign_expression() {
    if (array_index) {
        delete array_index;
    }
    delete assign_value;
}

generation_result assign_expression::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                              std::map<std::string, declared_identifier>& declared_identifiers,
                                              std::map<std::string, std::list<declaration*>*> struct_defs) {

    if (declared_identifiers.find(identifier) == declared_identifiers.end()) {
        std::string msg = "Undeclared identifier '" + identifier + "'";
        return generate_result(evaluate_error::undeclared_identifier, msg);
    }
    // evaluate the expression to be assigned to the given identifier
    generation_result res = assign_value->generate(result_instructions, declared_identifiers, struct_defs);
    if (res.result != evaluate_error::ok) {
        return res;
    }

    if (declared_identifiers.find(identifier) != declared_identifiers.end() &&
            declared_identifiers[identifier].type.parent_type == TYPE_META_STRUCT) {
        // assigning to a struct number
        int size = 0;

        // compute the offset
        for (auto def :*struct_defs[declared_identifiers[identifier].struct_name]){
            // check whether member identifier matches the definition identifier
            if (struct_member_identifier == def->identifier) {

                if (def->type != assign_value->get_type_info(declared_identifiers, struct_defs)) {
                    std::string msg = "Cannot assign value to member '" + struct_member_identifier + "' of '" + identifier + "' due to different data types";
                    return generate_result(evaluate_error::cannot_assign_type, msg);
                }

                break;
            }

            size += def->determine_size(struct_defs);
        }
        // new pcode_arg with the given identifier and summed size as offset
        auto arg = pl0_utils::pl0code_arg(identifier);
        arg.offset = size;
        result_instructions.emplace_back(pl0_utils::pl0code_fct::STO, arg);

        // is the assignment used as a right-hand side of another assignment?
        if (push_result_to_stack) {
            value tmp(identifier, struct_member_identifier);
            tmp.generate(result_instructions, declared_identifiers, struct_defs);
        }

    } else if (array_index == nullptr) {
        result_instructions.emplace_back(pl0_utils::pl0code_fct::STO, identifier);

        const auto& tp = declared_identifiers.find(identifier)->second.type;

        // if left-hand side is const, we cannot assign
        if (tp.is_const) {
            std::string msg = "Cannot assign value to constant '" + identifier + "'";
            return generate_result(evaluate_error::cannot_assign_const, msg);
        }

        // check if LHS type is the same as RHS type
        if (declared_identifiers.find(identifier)->second.type != assign_value->get_type_info(declared_identifiers, struct_defs)) {
            std::string msg = "Cannot assign value to variable '" + identifier + "' due to different data types";
            return generate_result(evaluate_error::cannot_assign_type, msg);
        }

        // is the assignment used as a right-hand side of another assignment?
        if (push_result_to_stack) {
            value tmp(new variable_ref(identifier.c_str()));
            tmp.generate(result_instructions, declared_identifiers, struct_defs);
        }
    } else {
        // array index is not null, an array is being accessed an array

        if (declared_identifiers.find(identifier)->second.type != assign_value->get_type_info(declared_identifiers, struct_defs)) {
            std::string msg = "Cannot assign value to variable '" + identifier + "' due to different data types";
            return generate_result(evaluate_error::cannot_assign_type, msg);
        }

        // specify the base
        result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);

        // evaluate the array index
        array_index->generate(result_instructions, declared_identifiers, struct_defs);

        // put the address of the start of the array to the top of the stack
        result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, pl0_utils::pl0code_arg(identifier));

        // add the index to the address representing the start of the array
        result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::ADD);

        // put the value to the memory specified by the address (first two values on the stack)
        result_instructions.emplace_back(pl0_utils::pl0code_fct::STA, 0, 0);

        // is the assignment used as a right-hand side of another assignment?
        if (push_result_to_stack) {
            value tmp(new variable_ref(identifier.c_str()), array_index);
            tmp.generate(result_instructions, declared_identifiers, struct_defs);
        }
    }

    return generate_result(evaluate_error::ok, "");

}