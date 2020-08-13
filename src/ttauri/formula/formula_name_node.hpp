// Copyright 2019, 2020 Pokitec
// All rights reserved.

#pragma once

#include "formula_node.hpp"

namespace tt {

struct formula_name_node final : formula_node {
    std::string name;
    mutable formula_post_process_context::function_type function;

    formula_name_node(parse_location location, std::string_view name) :
        formula_node(std::move(location)), name(name) {}

    void resolve_function_pointer(formula_post_process_context& context) override {
        function = context.get_function(name);
        if (!function) {
            TTAURI_THROW(parse_error("Could not find function {}()", name).set_location(location));
        }
    }

    datum evaluate(formula_evaluation_context& context) const override {
        ttlet &const_context = context;

        try {
            return const_context.get(name);
        } catch (error &e) {
            e.set_location(location);
            throw;
        }
    }

    datum &evaluate_lvalue(formula_evaluation_context& context) const override {
        try {
            return context.get(name);
        } catch (error &e) {
            e.set_location(location);
            throw;
        }
    }

    bool has_evaluate_xvalue() const override {
        return true;
    }

    /** Evaluate an existing xvalue.
    */
    datum const &evaluate_xvalue(formula_evaluation_context const& context) const override {
        try {
            return context.get(name);
        } catch (error &e) {
            e.set_location(location);
            throw;
        }
    }

    datum &assign(formula_evaluation_context& context, datum const &rhs) const override {
        try {
            return context.set(name, rhs);
        } catch (error &e) {
            e.set_location(location);
            throw;
        }
    }

    datum call(formula_evaluation_context& context, datum::vector const &arguments) const override {
        return function(context, arguments);
    }

    std::string get_name() const noexcept override {
        return name;
    }

    std::string string() const noexcept override {
        return name;
    }
};

}