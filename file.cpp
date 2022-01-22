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
    get_global_identifier_cell()["showMeNumba"] = BuiltinBase + 0;


    declare_identifier("showMeNumbaAdios", { TYPE_VOID, 0, true }, global_identifiers, "", false, {{TYPE_INT}});
    get_global_identifier_cell()["showMeNumbaAdios"] = BuiltinBase + 1;

    declare_identifier("showMeCharz", { TYPE_VOID, 0, true }, global_identifiers, "", false, {{TYPE_STRING}});
    get_global_identifier_cell()["showMeCharz"] = BuiltinBase + 2;

    declare_identifier("showMeCharzAdios", { TYPE_VOID, 0, true }, global_identifiers, "", false, {{TYPE_STRING}});
    get_global_identifier_cell()["showMeCharzAdios"] = BuiltinBase + 3;

    declare_identifier("gimmeNumba", { TYPE_INT, 0, true }, global_identifiers, "", false, {{}});
    get_global_identifier_cell()["gimmeNumba"] = BuiltinBase + 4;
}
