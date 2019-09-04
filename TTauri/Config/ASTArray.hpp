// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "ASTExpression.hpp"
#include "ASTExpressionList.hpp"

namespace TTauri::Config {

struct ASTArray : ASTExpression {
    std::vector<ASTExpression *> expressions;

    ASTArray(Location location) noexcept : ASTExpression(location), expressions() { }

    ASTArray(Location location, ASTExpressionList *expressions) noexcept : ASTExpression(location), expressions(expressions->expressions) {
        // We copied the pointers of the expression, so they must not be destructed when expressions is deleted.
        expressions->expressions.clear();
        delete expressions;
    }

    ~ASTArray() {
        for (auto expression: expressions) {
            delete expression;
        }
    }

    std::string string() const noexcept override {
        std::string s = "[";

        bool first = true;
        for (auto expression: expressions) {
            if (!first) {
                s += ",";
            }
            s += expression->string();
            first = false;
        }

        s += "]";
        return s;
    }

    datum execute(ExecutionContext &context) const override {
        datum::vector values;

        for (let expression: expressions) {
            values.push_back(expression->execute(context));
        }
        return datum{values};
    }

    /*! Execute an array-literal inside an object literal.
     * When a list literal is encountered inside an object literal it is
     * interpreted as a section-statement instead.
     *
     * A section-statement will change the currently active object for following
     * statements. A section-statment will select (and potentially create) a new object
     * inside the encapsulating object, then statements following this will be executed
     * on the newly created object.
     *
     * Every section statement will first reset the current active object to the encapsulating object
     * before selecting/creating a new object.
     *
     * An empty section-statement will reset the current active object to the encapsulating object.
     */
    void executeStatement(ExecutionContext &context) const override {
        if (expressions.size() == 0) {
            // empty section-statement; reset the currently active object.
            context.setSection({});

        } else if (expressions.size() == 1) {
            // section-statement with one expression; reset the currently active object.
            // Then find/create an object based on the expression, then select it as the
            // active object.
            context.setSection({});
            let selector = expressions.at(0)->getFQName();
            context.setSection(selector);

        } else {
            TTAURI_THROW(invalid_operation_error("syntax error, expected 0 or 1 expression in section statement")
                .set<"location"_tag>(location)
            );
        }
    }

};

}
