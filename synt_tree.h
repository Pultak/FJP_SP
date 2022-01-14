#include <iostream>

#include <stack>
#include <list>
#include <utility>
#include <vector>
#include <map>
#include "pl0_utils.h"


// the stack frame mechanism is built in a way that this number would eventually "converge" to global scope
// to speed things up, the interpreter immediatelly jumps to global scope base when this level value is seen
constexpr uint8_t LevelGlobal = 255;


// start address of the standard functions
constexpr int BuiltinBase = 0x00FFFFFF;

// call block size (each function call has this as a part of its stack frame)
// - it consists of outer context base, current base, return address and return value of callees
// this is retained from virtual p-machine definition and enhanced for return values
constexpr int CallBlockBaseSize = 4;
// cell of a return value (where the callee stores it in parent scope (level = 1), and caller takes it from there in his scope (level = 0))
constexpr int ReturnValueCell = 3;

enum class evaluate_error {
    ok,                             // all OK
    undeclared_identifier,          // identifier not declared
    undeclared_struct_member,       // struct does not contain member todo add structs
    unknown_typename,               // struct was not found todo add structs
    unresolved_reference,           // unknown symbol
    redeclaration,                  // identifier redeclaration
    redeclaration_different_types,  // function identifier redeclaration with different types (return type, parameters)
    invalid_call,                   // invalid function call
    cannot_assign_type,             // cannot assign -> incompatible type
    cannot_assign_const,            // cannot reassign const
    func_no_return,         // no return found in fuction
    invalid_state,                  // implementation error
};

struct declared_identifier;


struct generation_result{
public:
    evaluate_error result;
    std::string message;
};

generation_result generate_result(evaluate_error result, std::string message){
    return generation_result{result, std::move(message)};
}


struct global_statement {
    virtual generation_result generate(std::vector<pl0_utils::pl0code_instruction>& result_instructions,
                                       std::map<std::string, declared_identifier>& declared_identifiers,
                                       int& identifier_cell, int& frame_size) = 0;
};



struct declaration {
    declaration(int entity_major_type, const std::string& entity_identifier, int arraySize = 0)
            : type{ entity_major_type, 0 }, identifier(entity_identifier), array_size(arraySize) {
        //
    }

    declaration(int entity_major_type, std::string struct_name_identifier, std::string entity_identifier)
            : type{ entity_major_type, 0 }, identifier(entity_identifier), array_size(0), struct_name(struct_name_identifier) {
        //
    }

    std::string identifier;
    pl0_utils::pl0type_info type;
    int array_size;
    std::string struct_name;

    int override_frame_pos = std::numeric_limits<int>::max();

    // get size of the declaration for the stack frame
    int determine_size() const {
        int size = 0;
        // 1 cell needed for types like bool, char and int
        if (type.parent_type == TYPE_BOOL || type.parent_type == TYPE_CHAR || type.parent_type == TYPE_INT) {
            size = 1;
            if (array_size > 0) {
                size *= array_size;
            }
        }
        //todo define structs => their size can be variable

        return size;
    }

    generation_result generate(int& identifier_cell, int& frame_size) {

        //todo if struct => verifying struct existence

        // get size actual type
        int size = determine_size();

        // we did not override position
        if (override_frame_pos == std::numeric_limits<int>::max()) {
            identifier_cell = frame_size;
            frame_size += size;
        }
        else {
            // position override is done for function parameters
            identifier_cell = override_frame_pos;
        }
        //todo global variables

        return generate_result(evaluate_error::ok, "");
    }
};

// Already declared identifier wrapper structure
struct declared_identifier {
    // value type of the identifier
    pl0_utils::pl0type_info type = { TYPE_VOID, 0 };
    // if structure then specify name
    std::string struct_name{};
    // is forward declared?
    bool forward_declaration = false;
    // if function then specify function parameters
    std::vector<pl0_utils::pl0type_info> parameters = {};

    int identifier_address;
};




