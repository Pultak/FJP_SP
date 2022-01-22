//
// Created by pultak on 14.01.22.
//
#include "method.h"

method_declaration::method_declaration(declaration* fdecl, block* cmds, std::list<declaration*>* param_list)
        : decl(fdecl), code_block(cmds), parameters_list(param_list) {
    // full function definition with code block
    if (code_block) {
        code_block->is_function_block = true;
    }

    decl->type.is_function = true;
}

method_declaration::~method_declaration() {
    delete decl;
    delete code_block;
    delete parameters_list;
}

generation_result method_declaration::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                               std::map<std::string, declared_identifier>& declared_identifiers,
                                               int& frame_size, bool global) {
    bool forward_declared = (code_block == nullptr);

    std::vector<pl0_utils::pl0type_info> param_types;
    if (parameters_list && parameters_list->size() > 0) {
        for (auto* decl : *parameters_list) {
            param_types.push_back(decl->type);
        }
    }

    // declare new function identifier
    generation_result ret = declare_identifier(decl->identifier, decl->type,
                                               declared_identifiers, decl->struct_name,
                                               forward_declared, param_types);
    if (ret.result != evaluate_error::ok) {
        return ret;
    }

    // any command present for definition?
    if (!forward_declared) {

        // store the address this symbol is defined on
        declared_identifiers[decl->identifier].identifier_address = static_cast<int>(result_instructions.size());

        std::list<std::unique_ptr<variable_declaration>> param_decl;

        // parameters should be pushed to stack before function call
        // parameters are loaded into cells directly before called function stack frame
        // the callee then refers to parameters with negative cell index (-1 refers to last parameter, -2 to second to last, etc.)
        if (parameters_list) {
            int pos = -static_cast<int>(parameters_list->size());
            // transform "declaration" to "variable_declaration" in order to inject it to callee scope
            for (declaration* decl : *parameters_list) {
                decl->override_frame_pos = pos++;
                param_decl.emplace_back(std::make_unique<variable_declaration>(decl, false, nullptr));
            }
        }

        // reserve place for them and declare their identifiers in it
        code_block->injected_declarations = &param_decl;
        //todo what will happen after method destructor called? injected_declarations

        return_statement = false;
        ret = code_block->generate(result_instructions, declared_identifiers, return_statement);

        if (ret.result != evaluate_error::ok) {
            return ret;
        }

        // check if a non-void function returns a value
        if (!return_statement) {
            //todo should we use void functions?
            if (decl->type.parent_type != TYPE_VOID) {
                return generate_result(evaluate_error::func_no_return, "Function '" + decl->identifier + "' does not return any value");
            }
        }

        result_instructions.emplace_back(pl0_utils::pl0code_fct::RET, pl0_utils::pl0code_opr::RETURN);
    }

    return generate_result(evaluate_error::ok, "");
}

