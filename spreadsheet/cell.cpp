#include "cell.h"
#include "formula.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

using namespace std::literals;

// пустая ячейка ------------------------------------------------------------------

CellInterface::Value Cell::EmptyImpl::GetValue()
{
    return ""s;
}

std::string Cell::EmptyImpl::GetText() const
{
    return ""s;
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const
{
    return std::vector<Position> { };
}

void Cell::EmptyImpl::InvalidateCache()
{
    return;
}

bool Cell::EmptyImpl::IsCached() const
{
    return true;
}

// текстовая ячейка ----------------------------------------------------------------

Cell::TextImpl::TextImpl(const std::string &text) : text_{text} { }

CellInterface::Value Cell::TextImpl::GetValue() 
{
    if (text_.at(0) == ESCAPE_SIGN)
    {
        return text_.substr(1);
    }
    return text_;
}

std::string Cell::TextImpl::GetText() const
{
    return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const
{
    return std::vector<Position> { };
}

void Cell::TextImpl::InvalidateCache()
{
    return;
}

bool Cell::TextImpl::IsCached() const
{
    return true;
}

// формульная ячейка ---------------------------------------------------------------

Cell::FormulaImpl::FormulaImpl(const std::string &text, SheetInterface& sheet) :
    formula_(ParseFormula(std::move(text.substr(1)))),
    sheet_(&sheet) {
}

void Cell::FormulaImpl::Set(std::string text) {
    formula_ = ParseFormula(std::move(text.substr(1)));
}

CellInterface::Value Cell::FormulaImpl::GetValue() {
    

    if (!cache_value_) {
        //std::cout << "calculation" << std::endl; // для тестирования
        FormulaInterface::Value result = formula_->Evaluate(*sheet_);
        if (std::holds_alternative<double>(result))
        {
            cache_value_ = std::get<double>(result);
        }
        else if (std::holds_alternative<FormulaError>(result)) {
            cache_value_ = FormulaError(FormulaError::Category::Div0);
        }
        else {
            cache_value_ = std::get<FormulaError>(result);
        }
    }
    return *cache_value_;
}

std::string Cell::FormulaImpl::GetText() const 
{
    return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const
{
    return formula_.get()->GetReferencedCells();
}

Cell::Cell(SheetInterface& sheet, const Position& position) :
    impl_( std::make_unique<EmptyImpl>() ),
    sheet_(&sheet), position_(position) 
{

}

void Cell::FormulaImpl::InvalidateCache()
{
    cache_value_.reset();
}

bool Cell::FormulaImpl::IsCached() const
{
    return cache_value_.has_value();
}

// класс-обёртку Cell -------------------------------------------------------------------

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    // если текст не изменился - ничего не делаем
    if (text == impl_->GetText()) {
        return;
    }
    if (text.size() > 1 && text.at(0) == FORMULA_SIGN) {
        impl_ = std::make_unique<FormulaImpl>(text, *sheet_);
        UpdateGraphReference();
    } else if (text.size () > 0) {
        impl_ = std::make_unique<TextImpl>(text);
    } else {
        impl_ = std::make_unique<EmptyImpl>();
    }
}

void Cell::Clear() {
    impl_ = nullptr;
}

Cell::Value Cell::GetValue() const
{
    return impl_->GetValue();
}

std::string Cell::GetText() const
{
    return impl_->GetText();
}

Position Cell::GetPosition() const
{
    return position_;
}

std::vector<Position> Cell::GetReferencedCells() const
{
    if (impl_ != nullptr) {
        return impl_.get()->GetReferencedCells();
    }
    return std::vector<Position> { };
}

Cell::GraphReference& Cell::GetGraphReference()
{
    return graph_reference_;
}

void Cell::UpdateGraphReference()
{
    std::vector<Cell*> cells_referenced;
    if (sheet_->GetCell(this->position_)) {
        for (Position pos : GetReferencedCells()) {
            Cell* p_cell = static_cast<Cell*>(sheet_->GetCell(pos));
            cells_referenced.push_back(p_cell);
        }
    }
    graph_reference_.UpdateReferences(cells_referenced);
}

void Cell::InvalidateCache()
{
    impl_->InvalidateCache();
}

bool Cell::IsCacheValid() const
{
    return impl_->IsCached();
}

// GraphReference (граф связей) -------------------------------------------------

void Cell::GraphReference::AddDependency(CellInterface* cell)
{
    this->cells_dependent_.insert(cell);
}

void Cell::GraphReference::DeleteDependency(CellInterface* cell)
{
    cells_dependent_.erase(cell);
}

void Cell::GraphReference::AddReferences(CellInterface* cell)
{
    cells_referenced_.insert(cell);
}

void Cell::GraphReference::DeleteReferences(CellInterface* cell)
{
    cells_referenced_.erase(cell);
}

void Cell::GraphReference::UpdateReferences(std::vector<Cell*> cells_referenced)
{
    cells_referenced_.clear();
    for (Cell* cell : cells_referenced) {
        cells_referenced_.insert(cell);
    }
}

const std::vector<Cell*> Cell::GraphReference::GetReferences() const
{
    std::vector<Cell*> result;
    for (const auto cell : cells_referenced_) {
        if (cell) {
            result.push_back(static_cast<Cell*>(cell));
        }
    }
    return result;
}

const std::vector<Cell*> Cell::GraphReference::GetDependent() const
{
    std::vector<Cell*> result;
    for (const auto cell : cells_dependent_) {
        if (cell) {
            result.push_back(static_cast<Cell*>(cell));
        }
    }
    return result;
}

bool Cell::GraphReference::IsCyclicDependent(const Cell* start_cell,
            std::vector<Cell*> cells_referenced) const
{
    // Проверяем все зависимые ячейки
    for (Cell* ref_cell_ptr : cells_referenced)
    {
        if (!ref_cell_ptr) // ссылка на несуществующую ячейку
        {
            continue;
        }

        // Позиция ячейки совпадает с позицией ячейки из списка зависимости
        // (циклическая зависимость найдена)
        if (ref_cell_ptr == start_cell)
        {
            return true;
        }

        // Рекурсивно вызываем проверку для ячеек из списка зависимости текущей. 
        // start_ptr передаем без изменений

        Cell::GraphReference& graph_new_cell = (*static_cast<Cell*>(ref_cell_ptr)).GetGraphReference();
        // список указателей на ячейки, на которые ссылается новая ячейка
        std::vector<Cell*> cells_referenced = graph_new_cell.GetReferences();
        if (static_cast<Cell*>(ref_cell_ptr)->GetGraphReference().IsCyclicDependent(start_cell, cells_referenced))
        {
            return true;
        }
    }

    // Если мы здесь, циклические зависимости не найдены
    return false;
}

void Cell::GraphReference::InvalidateCacheDependent()
{
    // Для всех зависимых ячеек рекурсивно инвалидируем кэш
    for (Cell* dependent_cell : GetDependent())
    {
        // если кэш невалиден у текущей ячейки, то дальше "раскручивать" связи не нужно
        if (dependent_cell->IsCacheValid()) {
            dependent_cell->InvalidateCache();
            dependent_cell->GetGraphReference().InvalidateCacheDependent();
        }
    }
}