//
// Created by pultak on 13.01.22.
//

#include "code_block.h"

//todo check and change

//todo change
// declare identifier in current scope
generation_result declare_identifier(const std::string& identifier, const pl0_utils::pl0type_info type,
                                  std::map<std::string, declared_identifier>& declared_identifiers,
                                  const std::string& struct_name, bool forward_decl,
                                  const std::vector<pl0_utils::pl0type_info>& types) {

    auto itr = declared_identifiers.find(identifier);

    // identifier must not exist (at all - do not allow overlapping)
    if (itr == declared_identifiers.end()) {
        declared_identifiers[identifier] = {
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
    }

    return generate_result(evaluate_error::redeclaration, "Identifier '" + identifier + "' redeclaration");
}

bool find_identifier(const std::string& identifier, int& level, int& offset, block* scope){

    // look for identifier in scope
    if (scope) {
        auto res = scope->declared_identifiers.find(identifier);
        if (res != scope->declared_identifiers.end()) {
            level = 0;
            //todo address needed?
            offset = res->second.identifier_address;
            return true;
        }
    }
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

bool find_identifier(const std::string& identifier, int& level, int& offset,
                     std::map<std::string, declared_identifier> declared_identifiers) {

    //check local scope
    auto res = declared_identifiers.find(identifier);
    if (res != declared_identifiers.end()) {
        level = 0;
        offset = res->second.identifier_address;
        return true;
    }


    // look to global scope
    auto globalRes = global_identifier_cell.find(identifier);
    if (globalRes != global_identifier_cell.end()) {
        level = LevelGlobal;
        offset = globalRes->second;
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
        result_instructions.emplace_back(pl0_utils::pl0code_fct::STO, ReturnValueCell, 1); // return value is stored as first cell (index = 3) of caller scope (level = 1)
        result_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, pl0_utils::pl0code_opr::RETURN);

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
                                  bool& return_val) {

    int function_address = -1;

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
            //todo global identifiers needed too?
            decl->generate(result_instructions, declared_identifiers, frame_size, false);
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
                bool found = find_identifier(instr.arg.symbolref, lvl, val, this);
                if (found) {
                    // resolve if found
                    instr.arg.resolve(val);
                    instr.lvl = (instr.arg.isfunc) ? 0 : lvl;
                }
            }
        }

        // undeclare all identifiers
        for (auto& decl : declared_identifiers) {
            undeclare_identifier(decl.second.struct_name, declared_identifiers);
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
