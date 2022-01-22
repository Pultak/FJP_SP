//
// Created by pultak on 12.01.22.
//
#pragma once
#include <list>
#include <map>
#include <memory>

#include "synt_tree.h"
#include "value.h"

struct loop;
struct variable_declaration;
struct condition;
//todo namespace + refactoring in the future needed

struct block;


std::map<std::string, std::list<declaration*>*> get_struct_defs();
std::map<std::string, value*> get_global_initializers();
//todo change identifier cell to declared identifiers?
std::map<std::string, int> get_global_identifier_cell();

// declare identifier in current scope
generation_result declare_identifier(const std::string& identifier, const pl0_utils::pl0type_info type,
                                  std::map<std::string, declared_identifier>& declared_identifiers,
                                  const std::string& struct_name = "", bool forward_decl = false,
                                  const std::vector<pl0_utils::pl0type_info>& types = {}, bool global = false);

bool find_identifier(const std::string& identifier, int& level, int& offset,
                     std::map<std::string, declared_identifier> declared_identifiers, bool global);


// undeclare existing identifier
bool undeclare_identifier(const std::string& identifier,
                          std::map<std::string, declared_identifier>& declared_identifiers);


// any command
struct command {
    command(variable_declaration* decl): var(decl) {}

    command(expression* decl): expr(decl) {}

    command(value* val): command_value(val) {}

    command(loop* loop): cycle(loop) {}

    command(condition* cond): cond(cond) {}

    virtual ~command();

    variable_declaration* var = nullptr;
    expression* expr = nullptr;
    loop* cycle = nullptr;
    condition* cond = nullptr;
    value* command_value = nullptr;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers,
                               bool& return_val, int& frame_size);
};

// commands block like inside of function/condition/loop for example
struct block {
    block(std::list<command*>* commandlist) : commands(commandlist) {}
    virtual ~block();

    // list of block commands
    std::list<command*>* commands;

    // size of function frame
    int frame_size = CallBlockBaseSize; // call block and return value is implicitly present

    // map of declared identifiers in the block scope
    //std::map<std::string, declared_identifier> declared_identifiers;

    //identifiers list size on init
    int passed_identifiers_count;

    // is this block a function? Functions need a frame to allocate
    bool is_function_block = false;

    std::list<std::unique_ptr<variable_declaration>>* injected_declarations = nullptr; // for function parameters

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier> declared_identifiers,
                               bool& return_val);
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
    virtual generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers) = 0;
};

struct while_loop : public loop {
    while_loop(value* exp, block* commands)
            : loop(commands), cond_expr(exp) {
        //
    }
    virtual ~while_loop() {
        delete cond_expr;
    }

    // condition checked on each iteration
    value* cond_expr;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers) override {

        generation_result ret;

        // evaluate condition
        int condpos = static_cast<int>(result_instructions.size());
        ret = cond_expr->generate(result_instructions, declared_identifiers, get_struct_defs());
        if (ret.result != evaluate_error::ok) {
            return ret;
        }

        // jump below loop block if false (previous evaluation leaves result on stack, JPC performs jump based on this value)
        int jpcpos = result_instructions.size();
        result_instructions.emplace_back(pl0_utils::pl0code_fct::JPC, 0);

        // evaluate loop commands
        if (loop_commands) {
            bool dummy;
            ret = loop_commands->generate(result_instructions, declared_identifiers, dummy);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            // jump at condition and repeat, if needed
            result_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, condpos);
        }

        // store current "address" to JPC instruction, so it knows where to jump
        result_instructions[jpcpos].arg.value = static_cast<int>(result_instructions.size());

        return generate_result(evaluate_error::ok, "");
    }
};

struct for_loop : public loop {
    for_loop(expression* exp1, value* exp2, expression* exp3, block* commands)
            : loop(commands), init_expr(exp1), cond_val(exp2), mod_expr(exp3) {
        //
    }
    virtual ~for_loop() {
        delete init_expr;
        delete cond_val;
        delete mod_expr;
    }

    expression* init_expr, *mod_expr;
    value* cond_val;

    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers) override {

        generation_result ret;

        // evaluate initialization expression
        ret = init_expr->generate(result_instructions, declared_identifiers, get_struct_defs());
        if (ret.result != evaluate_error::ok) {
            return ret;
        }

        // store repeat position (we will jump here after each iteration)
        int repeatpos = static_cast<int>(result_instructions.size());

        // evaluate condition value
        ret = cond_val->generate(result_instructions, declared_identifiers, get_struct_defs());
        if (ret.result != evaluate_error::ok) {
            return ret;
        }

        // jump below loop block if false
        int jpcpos = result_instructions.size();
        result_instructions.emplace_back(pl0_utils::pl0code_fct::JPC, 0);

        // evaluate loop commands
        if (loop_commands) {
            bool dummy;
            ret = loop_commands->generate(result_instructions, declared_identifiers, dummy);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }
        }

        // evaluate modifying expression
        ret = mod_expr->generate(result_instructions, declared_identifiers, get_struct_defs());
        if (ret.result != evaluate_error::ok) {
            return ret;
        }

        // jump to condition
        result_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, repeatpos);

        // update JPC target position to jump here when the condition is not met
        result_instructions[jpcpos].arg.value = static_cast<int>(result_instructions.size());

        return generate_result(evaluate_error::ok, "");
    }
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
                               int& frame_size, bool global) override;
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

        generation_result ret = cond_expr->generate(result_instructions, declared_identifiers, get_struct_defs());
        if (ret.result != evaluate_error::ok) {
            return ret;
        }

        // prepare JPC, the later code will generate target address
        int jpc_position = result_instructions.size();
        result_instructions.emplace_back(pl0_utils::pl0code_fct::JPC, 0);
        int end_block = -1;

        // true commands (if condition is true)
        if (first_commands) {
            bool dummy;
            ret = first_commands->generate(result_instructions, declared_identifiers, dummy);
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

            bool dummy;
            ret = else_commands->generate(result_instructions, declared_identifiers, dummy);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }

            // if there is a false command block, true command block must jump after this
            result_instructions[end_block].arg.value = static_cast<int>(result_instructions.size());
        }

        return generate_result(evaluate_error::ok, "");
    }
};
