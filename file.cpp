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