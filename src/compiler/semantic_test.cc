#include "semantic.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "src/compiler/module.h"
#include "src/compiler/type_info.h"
#include "src/symbol.h"

using dub::compiler::Typer;
using dub::Module;
using dub::Symbol;

TEST(Typer, TypeExpressionConst) {
  Module mod = Module::WithName(Symbol::Get("testing"));
  // Typer typer({}, {}, mod.TypeInfo());
}
