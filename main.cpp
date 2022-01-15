#include <iostream>
#include <string>
#include <fstream>

#include "file.h"

extern int parse_code(FILE* input, FILE* output);
extern file* ast_root;

int main(int argc, char* argv[]){
    int parseresult = 0;
    std::string out_name;
    if (argc >= 3) {
        std::cout << "Opening source file " << argv[2] << std::endl;
        FILE* in_file = fopen(argv[2], "r");
        out_name = argv[1];

        if (!in_file) {
            std::cerr << "Cannot open file: " << in_file << std::endl;
            return -1;
        }

        std::cout << "Parsing..." << std::endl;
        parseresult = parse_code(in_file, stdout);

    } else if(argc == 2) {
        parseresult = parse_code(stdin, stdout);
        out_name = argv[1];
    }
    else{
        std::cout << "Not enough program arguments" << std::endl;
    }
    std::cout << "Parsing finished" << std::endl;
    if (parseresult != 0) {
        std::cerr << "Parsing error" << std::endl;
        return -1;
    }

    //semantic analysis + code generation

    std::cout << "Starting semantic analysis and pl0 generation" << std::endl;
    generation_result result = ast_root->generate();
    if (result.result == evaluate_error::ok){
        //default saving
        auto instr_out_name = out_name + ".instr";
        std::cout << "Compilation successful! Instructions written to: " << instr_out_name << std::endl;
        std::ofstream out(instr_out_name, std::ios::out);

        for (auto& instr : ast_root->generated_instructions) {
            out << instr.to_string() << std::endl;
        }
        out.flush();
        out.close();

        //binary instructions saving
        std::vector<pl0_utils::binary_instruction> binary_instructions;
        for (auto& instr : ast_root->generated_instructions) {
            pl0_utils::binary_instruction bi;
            bi.f = instr.instruction;
            bi.l = instr.lvl;
            bi.a = instr.arg.value;
            binary_instructions.push_back(bi);
        }
        std::ofstream binary_out(out_name, std::ios::out | std::ios::binary);
        for (auto& bi : binary_instructions) {
            binary_out.write(reinterpret_cast<const char*>(&bi), sizeof(pl0_utils::binary_instruction));
        }
        binary_out.flush();
        binary_out.close();

        std::cout << std::endl << "Binary (executable) output written to: " << out_name << std::endl;

    }
    else{
        std::cerr << "Compilation FAILED: " << result.message << std::endl;
    }

    return 0;
}
