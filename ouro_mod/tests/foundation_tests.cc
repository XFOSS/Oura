import ouro.foundation.lexer;
#include <cassert>

int main() {
    const char* msg = ouro::foundation::hello();
    assert(msg[0] == 'l');
    return 0;
}
