//
// Created by pultak on 13.01.22.
//

#include "code_block.h"


std::map<std::string, int> global_identifier_cell;
// global initializers table
std::map<std::string, value*> global_initializers;

std::map<std::string, std::list<declaration*>*> struct_defs;


std::map<std::string, std::list<declaration*>*> get_struct_defs() {
    return struct_defs;
}


std::map<std::string, value*> get_global_initializers() {
    return global_initializers;
}


std::map<std::string, int> get_global_identifier_cell() {
    return global_identifier_cell;
}

// declare identifier into map
generation_result declare_identifier(const std::string& identifier, const pl0_utils::pl0type_info type,
                                  std::map<std::string, declared_identifier>& declared_identifiers,
                                  const std::string& struct_name, bool forward_decl,
                                  const std::vector<pl0_utils::pl0type_info>& types, bool global) {

    if(global){

    }//else{
        auto itr = declared_identifiers.find(identifier);

        // identifier must not exist (at all - do not allow overlapping)
        if (itr == declared_identifiers.end()) {
            declared_identifiers[identifier] = {
                    true,
                    type,
                    struct_name,
                    forward_decl,
                    types
            };
            return generate_result(evaluate_error::ok, "");
        }
            // forward declaration does not trigger redeclaration error
        else if (itr->second.forward_declaration || forward_decl) {

            auto& decl = itr->second;

            // always check signature (return type and parameters)
            if (decl.type.parent_type != type.parent_type || decl.type.child_type != type.child_type
                || decl.parameters.size() != types.size()) {
                return generate_result(evaluate_error::redeclaration_different_types, "Identifier '" + identifier + "' redeclaration with different types (return value and/or parameters)");
            }
            for (size_t i = 0; i < types.size(); i++) {
                if (decl.parameters[i].parent_type != types[i].parent_type || decl.parameters[i].child_type != types[i].child_type) {
                    return generate_result(evaluate_error::redeclaration_different_types, "Identifier '" + identifier + "' redeclaration with different types (return value and/or parameters)");
                }
            }

            // if current declaration attempt is not a forward declaration, mark it in declared record
            if (!forward_decl) {
                decl.forward_declaration = false;
            }
            return generate_result(evaluate_error::ok, "");
        }else if(!itr->second.declared){
            itr->second.type = type;
            itr->second.struct_name = struct_name;
            itr->second.forward_declaration = forward_decl;
            itr->second.parameters = types;
            return generate_result(evaluate_error::ok, "");
        }
    //}


    return generate_result(evaluate_error::redeclaration, "Identifier '" + identifier + "' redeclaration");
}

bool find_identifier(const std::string& identifier, int& level, int& offset,
                     std::map<std::string, declared_identifier> declared_identifiers, bool global){

    // look for identifier in scope
    if (!declared_identifiers.empty()) {
        auto res = declared_identifiers.find(identifier);
        if (res != declared_identifiers.end()) {
            level = global? LevelGlobal : 0;
            offset = res->second.identifier_address;
            return true;
        }
    }
    //is it forward declared?
    auto res = global_identifier_cell.find(identifier);
    if (res != global_identifier_cell.end()) {
        level = LevelGlobal;
        offset = res->second;
        return true;
    }
    return false;
}

// undeclare existing identifier
bool undeclare_identifier(const std::string& identifier,
                          std::map<std::string, declared_identifier>& declared_identifiers) {
    if (declared_identifiers.find(identifier) != declared_identifiers.end()) {
        declared_identifiers.erase(identifier);
        return true;
    }

    return false;
}


command::~command() {
    if (var) {
        delete var;
    }
    else if (expr) {
        delete expr;
    }
    else if (cycle) {
        delete cycle;
    }
    else if (cond) {
        delete cond;
    }
}
generation_result command::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                    std::map<std::string, declared_identifier>& declared_identifiers,
                                    bool& return_val, int& frame_size){

    generation_result ret;
    if (var) {
        ret = var->generate(result_instructions, declared_identifiers, frame_size, false);
    }
    else if (expr) {
        ret = expr->generate(result_instructions, declared_identifiers, struct_defs);
    }
    else if (cycle) {
        ret = cycle->generate(result_instructions, declared_identifiers);
    }
    else if (cond) {
        ret = cond->generate(result_instructions, declared_identifiers);
    }
    else if (command_value) {

        ret = command_value->generate(result_instructions, declared_identifiers, struct_defs);

        // return value of function is on top of the stack
        result_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 1, ReturnValueCell); // return value is stored as first cell (index = 3) of caller scope (level = 1)
        result_instructions.emplace_back(pl0_utils::pl0code_fct::RET, pl0_utils::pl0code_opr::RETURN);

        return_val = true;
    }
    else {
        ret = generate_result(evaluate_error::invalid_state, "Invalid state - symbol 'command' could not be evaluated in given context");
    }

    return ret;
}

block::~block() {
    if (commands) {
        delete commands;
    }
}

generation_result block::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                  std::map<std::string, declared_identifier> declared_identifiers,
                                  bool& return_val) {

    int function_address = -1;
    this->passed_identifiers_count = declared_identifiers.size();
    // if this is a function scope, prepare "frame allocation mark"
    // this will be filled later with "CallBlockBaseSize + size of local variables"
    // non-function scopes (like a block in condition/loop/...) do not allocate stack frames
    if (is_function_block) {
        function_address = result_instructions.size();
        result_instructions.emplace_back(pl0_utils::pl0code_fct::INT, 0);
    }

    // if outer scope decided to inject some declarations, evaluate them here
    // this is typically just a case of function call parameters (must be declared in callee context)
    if (injected_declarations) {
        for (auto& decl : *injected_declarations) {
            auto ret = decl->generate(result_instructions, declared_identifiers, frame_size, false);
            continue;
        }
    }

    // evaluate commands if any (empty block does not have commands)
    if (commands) {
        for (command* cmd : *commands) {
            generation_result ret = cmd->generate(result_instructions, declared_identifiers, return_val, this->frame_size);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }
        }
    }

    // if this is a function scope, perform some more operations
    if (is_function_block) {
        // fill stack frame allocation instruction with actual frame size
        result_instructions[function_address].arg.value = frame_size;

        int lvl, val;

        // resolve symbols used in this block
        // this must resolve variable symbols, but does not have to resolve functions
        // functions may have been forward-declared, in this case, "program::evaluate" will resolve them later
        for (int pos = function_address; pos < result_instructions.size(); pos++) {
            auto& instr = result_instructions[pos];

            // if the argument is a reference, try to resolve
            if (instr.arg.isref) {
                // find identifier
                //todo can be block global? if yes then change false -> some variable
                bool found = find_identifier(instr.arg.symbolref, lvl, val, declared_identifiers, false);
                if (found) {
                    // resolve if found
                    instr.arg.resolve(val);
                    instr.lvl = (instr.arg.isfunc) ? 0 : lvl;
                }
            }
        }
        int to_clean = declared_identifiers.size() - this->passed_identifiers_count;
        // undeclare all identifiers
        for(auto it = declared_identifiers.rbegin(); it != declared_identifiers.rend() && to_clean > 0; ++it, --to_clean){
            undeclare_identifier(it->second.struct_name, declared_identifiers);
        }
    }

    return generate_result(evaluate_error::ok, "");
}

loop::~loop() {
    delete loop_commands;
}

variable_declaration::~variable_declaration() {
    if (decl) {
        delete decl;
    }
    if (initialized_by) {
        delete initialized_by;
    }
}


condition::~condition() {
    delete cond_expr;
    delete first_commands;
    if (else_commands) {
        delete else_commands;
    }
}

generation_result variable_declaration::generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                           std::map<std::string, declared_identifier>& declared_identifiers,
                           int& frame_size, bool global) {

    //todo will same identifiers rewrite?
    declared_identifiers[decl->identifier] = {};
    auto& id = declared_identifiers.at(decl->identifier);
    generation_result ret = decl->generate(id.identifier_address, frame_size, global, get_struct_defs());
    if (ret.result != evaluate_error::ok) {
        return ret;
    }

    // declare identifier if not previously declared
    ret = declare_identifier(decl->identifier, decl->type, declared_identifiers, decl->struct_name);
    if (ret.result != evaluate_error::ok) {
        return ret;
    }

    // is this declaration initialized?
    if (initialized_by) {
        if(global){
            global_initializers[decl->identifier] = initialized_by;
        }else{
            ret = initialized_by->generate(result_instructions, declared_identifiers, get_struct_defs());
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            if (initialized_by->get_type_info(declared_identifiers, get_struct_defs()) != decl->type) {
                return generate_result(evaluate_error::cannot_assign_type, "Initializer type of '" + decl->identifier + "' does not match the variable type");
            }

            // store evaluation result (stack top) to a given variable
            result_instructions.emplace_back(pl0_utils::pl0code_fct::STO, decl->identifier);
        }
    }
    return generate_result(evaluate_error::ok,"");

}
