#ifndef __LOX_FUNCTION_H
#define __LOX_FUNCTION_H

#include <any>
#include <string>
#include <vector>

#include "LoxCallable.h"

class Environment;
class FunctionStmt;

class LoxFunction : public LoxCallable {
public:
    std::shared_ptr<FunctionStmt> declaration;
    std::shared_ptr<Environment> closure;

    LoxFunction(
            std::shared_ptr<FunctionStmt> declaration,
            std::shared_ptr<Environment> closure);

    int arity() override;
    std::any call(Interpreter& interpreter, 
            std::vector<std::any> arguments) override;
    std::string toString() override;
};

#endif /* __LOX_FUNCTION_H */
