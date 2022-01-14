//
// Created by pultak on 12.01.22.
//
#ifndef FJP_SP_CODE_BLOCK_H
#define FJP_SP_CODE_BLOCK_H

#include <list>
#include <map>

#include "synt_tree.h"
#include "value.h"

struct loop;
struct variable_declaration;
struct condition;
//todo namespace + refactoring in the future needed

// declare identifier in current scope
generation_result declare_identifier(const std::string& identifier, const pl0_utils::pl0type_info type,
                                  std::map<std::string, declared_identifier>& declared_identifiers,
                                  const std::string& struct_name = "", bool forward_decl = false,
                                  const std::vector<pl0_utils::pl0type_info>& types = {});

// undeclare existing identifier
bool undeclare_identifier(const std::string& identifier,
                          std::map<std::string, declared_identifier>& declared_identifiers);

bool find_identifier(const std::string& identifier, int& level, int& offset);


// any command
struct command {
    command(variable_declaration* decl)
            : var(decl) {
        //
    }

    command(expression* decl)
            : expr(decl) {
        //
    }

    command(value* val)
            : command_value(val) {
        //
    }

    command(loop* loop)
            : cycle(loop) {
        //
    }

    command(condition* cond)
            : cond(cond) {
        //
    }

    virtual ~command();

    variable_declaration* var = nullptr;
    expression* expr = nullptr;
    loop* cycle = nullptr;
    condition* cond = nullptr;
    value* command_value = nullptr;

    generation_result generate();
};

// commands block like inside of function/condition/loop for example
struct block {
    block(std::list<command*>* commandlist) : commands(commandlist) {
    }
    virtual ~block();

    // list of block commands
    std::list<command*>* commands;

    // size of function frame
    int frame_size = CallBlockBaseSize; // call block and return value is implicitly present

    // map of declared identifiers in the block scope
    std::map<std::string, declared_identifier> declared_identifiers;

    // is this block a function? Functions need a frame to allocate
    bool is_function_block = false;

    std::list<variable_declaration*>* injected_declarations = nullptr; // for function parameters

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers);
};


// loop definition
struct loop{
    loop(block* commands)
            : loop_commands(commands) {
        //
    };
    virtual ~loop();

    // commands inside this loop
    block* loop_commands;
};



// any variable declaration
struct variable_declaration : public global_statement {
    variable_declaration(declaration* var_declaration, bool constant = false, value* initializer = nullptr)
            : decl(var_declaration), is_constant(constant), initialized_by(initializer) {
        if (is_constant) {
            decl->type.is_const = true;
        }
    }
    virtual ~variable_declaration();

    declaration* decl = nullptr;
    bool is_constant;
    value* initialized_by;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers,
                               int& identifier_cell, int& frame_size) override {

        generation_result ret = decl->generate(identifier_cell, frame_size);
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
            ret = initialized_by->generate();
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            if (initialized_by->get_type_info() != decl->type) {
                return generate_result(evaluate_error::cannot_assign_type, "Initializer type of '" + decl->identifier + "' does not match the variable type");
            }

            // store evaluation result (stack top) to a given variable
            result_instructions.emplace_back(pl0_utils::pl0code_fct::STO, decl->identifier);
            //todo global initializers
        }
        return generate_result(evaluate_error::ok,"");

    }
};

// condition structure block
struct condition{
    condition(value* exp, block* commands, block* elsecommands = nullptr)
            : cond_expr(exp), first_commands(commands), else_commands(elsecommands) {
        //
    }
    virtual ~condition();

    // condition to be evaluated
    value* cond_expr;
    // true and false command blocks
    block* first_commands, *else_commands;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers){

        generation_result ret = cond_expr->generate();
        if (ret.result != evaluate_error::ok) {
            return ret;
        }

        // prepare JPC, the later code will generate target address
        int jpc_position = result_instructions.size();
        result_instructions.emplace_back(pl0_utils::pl0code_fct::JPC, 0);
        int end_block = -1;

        // true commands (if condition is true)
        if (first_commands) {
            ret = first_commands->generate(result_instructions, declared_identifiers);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            // jump below else block if any (do not execute "else" block)
            if (else_commands) {
                end_block = result_instructions.size();
                result_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, 0);
            }
        }

        // JPC will always jump here if the condition is false (it will either execute the false block or continue)
        result_instructions[jpc_position].arg.value = static_cast<int>(result_instructions.size());

        if (else_commands) {

            ret = else_commands->generate(result_instructions, declared_identifiers);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            // if there is a false command block, true command block must jump after this
            result_instructions[end_block].arg.value = static_cast<int>(result_instructions.size());
        }

        return generate_result(evaluate_error::ok, "");
    }
};



#endif //FJP_SP_CODE_BLOCK_H
