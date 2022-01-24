//
// Created by pultak on 15.01.22.
//
#include "file.h"

file::~file() {
    for (auto* stmt : *statements) {
        delete stmt;
    }
    delete statements;
}

void file::init_io_functions() {
    declare_identifier("showMeNumba", { TYPE_VOID, 0, true }, global_identifiers, "", false, {{TYPE_INT}});
    int print_function_addr = generated_instructions.size();
    global_identifiers["showMeNumba"].identifier_address = print_function_addr;
    get_global_identifier_cell()["showMeNumba"] = print_function_addr;

    //cycle init + get function argument
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::INT, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, -1);

    //body of the cycle
    //get last digit and write in ASCII
    int cycle_start_address = generated_instructions.size();
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 10);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 6);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 48);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 2);
    //generated_instructions.emplace_back(pl0_utils::pl0code_fct::WRI, 0);

    //now remove the last digit from the number
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 10);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 5);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 4);


    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 1);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 2);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 3);

    //if there is no more digits of our number then end
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMC, print_function_addr + 20);
    //do the cycle again
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, cycle_start_address);

    //write all numbers
    int write_numbers = generated_instructions.size();
    //get number count
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 3);
    //if index is 0 -> all done
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMC, write_numbers + 8);
    //else decrease by one and write one character
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 1);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::WRI, 0);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, write_numbers);

    //now write new line and return from procedure
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 13);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::WRI, 0);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 10);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::WRI, 0);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::RET, 0);

    declare_identifier("gimmeNumba", { TYPE_INT, 0, true }, global_identifiers, "", false);
    int scan_function_addr = generated_instructions.size();
    global_identifiers["gimmeNumba"].identifier_address = scan_function_addr;
    get_global_identifier_cell()["gimmeNumba"] = scan_function_addr;

    //cycle init + get function argument
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::INT, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 0);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 3);
    //read character
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::REA, 0);

    // is the character a new line?
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 10);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMC, scan_function_addr + 20);

    //is the character a white space?
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 22);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMC, scan_function_addr + 20);

    //now store as value
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 16);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 3);

    //multiple the result by 10 and add to it our actual value
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LOD, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::LIT, 10);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 4);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::OPR, 2);
    //jump to start
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::JMP, scan_function_addr + 3);

    //return value and end
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::INT, -1);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::STO, 1, 3);
    generated_instructions.emplace_back(pl0_utils::pl0code_fct::RET, 0);






}
