#include <iostream>
#include "table.h"


int main() {
    table t;
    t.add_block();
    t.print_table();

    // Simular algumas ações
    t.block_left();
    t.print_table();

    t.rotate_block();
    t.print_table();

    t.block_descend();
    t.print_table();

    t.block_drop();
    t.print_table();

    return 0;
}