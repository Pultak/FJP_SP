#pragma once

#include <memory>
#include "synt_tree.h"
#include "code_block.h"


// function declaration structure
struct method_declaration : public global_statement {

    method_declaration(declaration* fdecl, block* cmds, std::list<declaration*>* param_list = nullptr);
    virtual ~method_declaration();

    declaration* decl;
    block* code_block;
    std::list<declaration*>* parameters_list;
    // method flag indicating that return statement is present
    bool return_statement = false;


    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                               std::map<std::string, declared_identifier>& declared_identifiers,
                               int& frame_size, bool global) override;
};
 //FJP_SP_METHOD_H
