#pragma once
#include <list>
#include <string>
#include <vector>
#include <stack>

#include "code_block.h"
#include "method.h"
#include "struct.h"

//todo add structure parsing

struct file{
public:
    file(std::list<global_statement*>* stmt_list)
            : statements(stmt_list) {
        //
    }
    virtual ~file();

    std::list<global_statement*>* statements;
    std::stack<block*> code_scopes;

    // map of declared identifiers in the global scope
    std::map<std::string, declared_identifier> global_identifiers;


    std::vector<pl0_utils::pl0code_instruction> generated_instructions;
    std::string generation_message;

    int size_global_frame = 0;


    generation_result generate() {

        //todo global functions for the needed print enxtension

        generated_instructions.emplace_back(pl0_utils::pl0code_fct::INT, 0, 0);

        //reserve frame size for virtual callblock and main return value
        size_global_frame += CallBlockBaseSize;

        // jump instruction for global variables initialization
        generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, 0);

        // calling main function
        generated_instructions.emplace_back(pl0_utils::pl0code_fct::CAL, pl0_utils::pl0code_arg("main", true));
        generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, 0);
        //todo is the position always static? 3th instruction is main jump
        generated_instructions[3].arg.value = 3;

        for (auto* stmt : *statements) {
            std::map<std::string, declared_identifier> dummy;
            generation_result ret = stmt->generate(generated_instructions, dummy, size_global_frame, true);
            if (ret.result != evaluate_error::ok) {
                return ret;
            }
        }

        int lvl, val;

        //todo always static instruction position?
        generated_instructions[0].arg.value = size_global_frame;

        generated_instructions[1].arg.value = static_cast<int>(generated_instructions.size());

        for (auto& inits : get_global_initializers()) {
            //todo global_identifiers can make problems
            inits.second->generate(generated_instructions, global_identifiers, get_struct_defs());
            find_identifier(inits.first, lvl, val, nullptr);
            generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, val, lvl);
        }
        //todo is the position always static? 2th instruction is init jump
        generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, 1 + 1); // jump back

        //todo do we want string extension?

        // resolve functions address in all program instructions
        for (int pos = 0; pos < generated_instructions.size(); pos++) {
            auto& instr = generated_instructions[pos];
            if (instr.instruction == pl0_utils::pl0code_fct::CAL && instr.arg.isref && instr.arg.isfunc) {
                if (find_identifier(instr.arg.symbolref, lvl, val, nullptr)) {
                    instr.arg.resolve(val);
                    instr.lvl = 0;
                }
            }
        }

        //were all function addresses found?
        for (int pos = 0; pos < generated_instructions.size(); pos++) {
            auto& instr = generated_instructions[pos];
            if (instr.arg.isref) {
                return generate_result(evaluate_error::unresolved_reference, "Unresolved symbol '" + instr.arg.symbolref +"'");
            }
        }

        return generate_result(evaluate_error::ok, "");
    }

};
