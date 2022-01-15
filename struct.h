#pragma once
#include "synt_tree.h"
#include "code_block.h"

struct struct_definition : public global_statement {
    struct_definition(std::string struct_name_identifier, std::list<declaration*>* multi_decl)
            : struct_name(struct_name_identifier), contents(multi_decl){
        //
    }
    virtual ~struct_definition();

    std::string struct_name;
    std::list<declaration*>* contents;
    int last_struct_index = 0;


    generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                    std::map<std::string, declared_identifier>& declared_identifiers,
                                    int& frame_size, bool global) override {

        // define struct with given name and contents
        if (struct_defs.find(struct_name) == struct_defs.end()) {
            struct_defs[struct_name] = contents;
            //is not null and is not empty
            if(contents && (*contents).empty()){
                ++last_struct_index;
                for(auto& decl: *contents){
                    decl->type_indice = last_struct_index;
                }
            }else{
                //todo check if good?
                return generate_result(evaluate_error::unknown_typename, "Unknown type name '" + struct_name + "'");
            }

        }
        return generate_result(evaluate_error::ok, "");
    }
};
//FJP_SP_STRUCT_H
