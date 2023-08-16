#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы: 
    explicit Formula(std::string expression) try : ast_( ParseFormulaAST(expression) ) {
     }
    catch (std::exception& exc) {
        std::throw_with_nested(FormulaException(exc.what()));
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        }
        catch (FormulaError& err) {
            return err;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        // Вектор результатов
        std::vector<Position> result;

        // Убираем дублирование ячеек
        std::forward_list<Position> unique_ref_cells( ast_.GetCells() );
        unique_ref_cells.sort();
        unique_ref_cells.unique();
        for (const Position& cell : unique_ref_cells)
        {
            result.push_back(cell);
        }
        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}