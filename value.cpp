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
        case value_type::array_element:
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


pl0_utils::pl0type_info value::get_type_info() const
{

    auto get_identifier_type_info = [&context](const std::string& identifier) -> pl0_utils::pl0type_info {
        if (!is_identifier_declared(identifier)) {
            return { TYPE_VOID, 0, false };
        }

        return get_declared_identifier(identifier).type;
    };

    switch (value_type) {
        case value_type::const_number_lit:
            return { TYPE_INT, 0, false };
        case value_type::const_string_lit:
            return { TYPE_STRING, 0, false };
        case value_type::variable:
            return get_identifier_type_info(content.variable->identifier);
        case value_type::array_element:
            return get_identifier_type_info(content.array_element.array->identifier);
        case value_type::assign_expression:
            return content.assign_expr->assign_value->get_type_info();
            break;
        case value_type::boolean_expression:
            return { TYPE_BOOL, 0, false };
        case value_type::member:
        {
            // get name based on the identifier
            if (!is_identifier_declared(str_base)) {
                return { TYPE_VOID, 0, false };
            }
            const auto& def = get_declared_identifier(str_base);

            // load the struct definition
            auto struct_def = get_struct_definition(def.struct_name);

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
            pl0_utils::pl0type_info positivetype = content.ternary.positive->get_type_info();
            pl0_utils::pl0type_info negativetype = content.ternary.negative->get_type_info();

            if (positivetype != negativetype) {
                return { TYPE_VOID, 0, false };
            }

            return positivetype;
        }
        case value_type::return_value:
            return get_identifier_type_info(content.call->function_identifier);
    }

    return { TYPE_VOID, 0, false };
}


generation_result value::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                  std::map<std::string, declared_identifier>& declared_identifiers) {

    // pushing value to stack, unless "return_value_ignored" is defined

    generation_result ret;
    int address;

    switch (value_type) {
        case value_type::const_string_lit:
            // constant string literal - push literal address to stack
            address = context.store_global_string_literal(str_base);
            context.gen_instruction(pcode_fct::LIT, addr);
            break;
        case value_type::const_number_lit:
            // constant integer literal - push constant to stack
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, content.int_value);
            break;
        case value_type::variable:
            // variable - load by address
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, content.variable->identifier);
            ret = content.variable->generate();
            break;
        case value_type::arithmetic:
            // arithmetic expression - just evaluate, arithmetic_expression evaluate method will leave result on stack
            ret = content.arithmetic_expression->generate();
            break;
        case value_type::member: {
            // struct member - determine struct base and member offset and load it
            // checks if identifier is declared
            if (!is_identifier_declared(str_base)) {
                ret = generate_result(evaluate_error::undeclared_identifier, "Undeclared identifier '" + str_base + "'");
                break;
            }

            // get the struct name from identifier
            auto struct_name = declared_identifiers[str_base].struct_name;

            // struct must be defined
            if (!is_struct_defined(struct_name)) {
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
                size += member->determine_size();
            }

            // the member was not found
            if (!found) {
                ret = generate_result(evaluate_error::undeclared_struct_member, "Undeclared struct member '" + member_spec + "'");
                break;
            }

            // new pcode_arg with the size as a offset
            auto p_arg = pl0_utils::pl0code_arg(str_base);
            p_arg.offset = size;

            //todo level and argument swapped
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 0, p_arg);

            break;
        }
        case value_type::array_member:
            // array element - load base, move by offset and load using extended p-code instruction set
            // specify the base
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);

            // evaluate index value
            ret = content.array_element.index->generate();
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
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LDA,0,0);
            break;
        case value_type::ternary:{
            // ternary operator - evaluate condition and push value to stack according to boolean_expression result
            // evaluate conditional
            ret = content.ternary.boolexpr->generate();
            if (ret.result != evaluate_error::ok) {
                break;
            }
            // jump to false branch if false (address will be stored later, once known)
            int jpcpos = result_instructions.size();
            result_instructions.emplace_back(pl0_utils::pl0code_fct::JPC, 0);
            // evaluate "true" branch
            ret = content.ternary.positive->generate();
            if (ret.result != evaluate_error::ok) {
                break;
            }
            // jump below "false" branch
            int jmppos = result_instructions.size();
            result_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, 0);
            // fill in the "false" JPC jump address

            result_instructions[jpcpos].arg.value = static_cast<int>(result_instructions.size());
            // evaluate "false" branch
            ret = content.ternary.negative->generate();
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
            ret = content.assign_expr->generate();
            break;
            // boolean expression - evaluate, result is stored to stack (0 or 1)
        case value_type::boolean_expression:
            ret = content.boolexpr->generate();
            break;
        case value_type::return_value:
            // return value - evaluate the call, caller will store return value to a specific location
            ret = content.call->generate();
            if (ret.result != evaluate_error::ok) {
                break;
            }
            // return value is at 0;ReturnValueCell (callee put it there)
            result_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, ReturnValueCell, 0);
            break;
    }

    // if evaluated ok, but outer context decided to ignore return value, move stack pointer
    if (ret.result == evaluate_error::ok && return_value_ignored) {
        result_instructions.emplace_back(pl0_utils::pl0code_fct::INT, -1);
    }

    return ret;
}