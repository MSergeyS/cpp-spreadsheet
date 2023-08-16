#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for SetCell()");
    }

    // проверяем размер sheet, если нужно увеличиваем
    ResizeSheet(pos);

    // создаём новую ячейку, разбираем связи (если они есть) из text,
    // но пока не заносим её в таблицу
    std::unique_ptr<Cell> p_new_cell = PreCreateNewCell(pos, text);

    // старая ячейка - (ячейка с pos в таблице)
    Cell* old_cell = static_cast<Cell*>(GetCell(pos));
    // если ячейка существует  
    if (old_cell) { 
        // получаем список указателей на ячейки, на которые ссылается новая ячейка   
        Cell::GraphReference& graph_new_cell = (*p_new_cell.get()).GetGraphReference();
        std::vector<Cell*> cells_referenced = graph_new_cell.GetReferences();

        // проверяем на циклические зависимости новое содержимое cell
        if (graph_new_cell.IsCyclicDependent(old_cell, cells_referenced) ) {
            throw CircularDependencyException("Circular dependency detected!");
        }

        // Инвалидируем кэш этой ячейки и всех зависимых
        InvalidateCell(pos);
        
        // переносим информацию зависимостях в новую ячейку
        Cell::GraphReference graph_old_cell = (*old_cell).GetGraphReference();
        for ( CellInterface* cell_dep : graph_old_cell.GetDependent()) {
            // добавляем зависимость в новую ячейку
            graph_new_cell.AddDependency(cell_dep);
            // в зависимой ячейки удаляем ссылку на старую ячейку
            static_cast<Cell*>(cell_dep)->GetGraphReference().DeleteReferences(old_cell);
        }
    }

    // заносим ячейку в таблицу
    sheet_.at(pos.row).at(pos.col).swap(p_new_cell);
    Cell* new_cell = static_cast<Cell*>(sheet_.at(pos.row).at(pos.col).get());

    // обнавляем cсылки
    UpdatesReferences(new_cell);
   
    // изменяем, если нужно, минимальную печатную область
    max_col_ = (max_col_ < (pos.col + 1) ? pos.col + 1 : max_col_);
    max_row_ = (max_row_ < (pos.row + 1) ? pos.row + 1 : max_row_);  
 
}

CellInterface* Sheet::GetCell(Position pos)
{
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for GetCell()");
    }

    if (!CellExists(pos))
    {
        return nullptr;
    }

    if (sheet_.at(pos.row).at(pos.col).get() == nullptr) {
        return nullptr;
    }
    return sheet_.at(pos.row).at(pos.col).get();
}

const CellInterface* Sheet::GetCell(const Position pos) const {  
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for GetCell()");
    }

    if (!CellExists(pos))
    {
        return nullptr;
    }

    if (sheet_.at(pos.row).at(pos.col).get() == nullptr) {
        return nullptr;
    }
    return sheet_.at(pos.row).at(pos.col).get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for ClearCell()");
    }

    if (CellExists(pos))
    {   
        sheet_.at(pos.row).at(pos.col).reset();
    }

    if ((pos.row + 1 == max_row_) || (pos.col + 1 == max_col_))
    {
        // Удаленная ячейка была на границе Printable Area. Нужен перерасчет
        UpdatePrintableSize();
    }
}

Size Sheet::GetPrintableSize() const {
    return Size{ max_row_, max_col_ };
}

void Sheet::PrintValues(std::ostream& output) const
{
    for (int x = 0; x < max_row_; ++x)
    {
        bool need_separator = false;
        // Проходим по всей ширине Printable area
        for (int y = 0; y < max_col_; ++y)
        {
            // Проверка необходимости печати разделителя
            if (need_separator)
            {
                output << '\t';
            }
            need_separator = true;

            // Если мы не вышли за пределы вектора И ячейка не nullptr
            if ((y < static_cast<int>(sheet_.at(x).size())) && sheet_.at(x).at(y))
            {
                // Ячейка существует
				std::visit(OstreamSolutionPrinter { output },
                        sheet_.at(x).at(y)->GetValue());
            }
        }
        // Разделение строк
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const
{
    for (int x = 0; x < max_row_; ++x)
    {
        bool need_separator = false;
        // Проходим по всей ширине Printable area
        for (int y = 0; y < max_col_; ++y)
        {
            // Проверка необходимости печати разделителя
            if (need_separator)
            {
                output << '\t';
            }
            need_separator = true;

            // Если мы не вышли за пределы вектора И ячейка не nullptr
            if ((y < static_cast<int>(sheet_.at(x).size())) && sheet_.at(x).at(y))
            {
                // Ячейка существует
                output << sheet_.at(x).at(y)->GetText();
            }
        }
        // Разделение строк
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

void Sheet::ResizeSheet(const Position pos)
{
    if (!pos.IsValid())
    {
        return;
    }

    // Если элементов в векторе строк меньше, чем номер строки в pos.row
    if (static_cast<int>(sheet_.size()) < (pos.row + 1))
    {
        // резервируем и инициализируем nullptr элементы вплоть до строки pos.row
        sheet_.reserve(pos.row + 1);
        sheet_.resize(pos.row + 1);
    }

    // Если элементов в векторе столбцов меньше, чем номер столбца в pos.col
    if (static_cast<int>(sheet_.at(pos.row).size()) < (pos.col + 1))
    {
        // резервируем и инициализируем nullptr элементы вплоть до столбца pos.col
        sheet_.at(pos.row).reserve(pos.col + 1);
        sheet_.at(pos.row).resize(pos.col + 1);
    }
}

bool Sheet::CellExists(Position pos) const
{
    return (pos.row < static_cast<int>(sheet_.size())) &&
           (pos.col < static_cast<int>(sheet_.at(pos.row).size()));
}

void Sheet::UpdatePrintableSize()
{
    max_row_ = 0;
    max_col_ = 0;

    // Сканируем ячейки, пропуская nullptr
    for (int x = 0; x < static_cast<int>(sheet_.size()); ++x)
    {
        for (int y = 0; y < static_cast<int>(sheet_.at(x).size()); ++y)
        {
            if (sheet_.at(x).at(y))
            {
                max_row_ = (max_row_ < (x + 1) ? x + 1 : max_row_);
                max_col_ = (max_col_ < (y + 1) ? y + 1 : max_col_);
            }
        }
    }
}

Cell* Sheet::PositionToCell(Position pos) const {
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for GetCell()");
    }

    if (!CellExists(pos))
    {
        return nullptr;
    }

    return sheet_.at(pos.row).at(pos.col).get();
}

Cell* Sheet::AddEmptyCell(const Position pos) {
    ResizeSheet(pos);
    // создаём умный указатель на cell
    std::unique_ptr<Cell> new_cell = std::make_unique<Cell>(*this, pos);
    sheet_.at(pos.row).at(pos.col).swap(new_cell);
    return  sheet_.at(pos.row).at(pos.col).get();
}

void Sheet::InvalidateCell(const Position& pos)
{
    Cell* cell = static_cast<Cell*>(GetCell(pos));
    cell->InvalidateCache();
    cell->GetGraphReference().InvalidateCacheDependent();
}

std::unique_ptr<Cell> Sheet::PreCreateNewCell(const Position pos, const std::string text) {

    // разбираем новое содержимое
    // создаём умный указатель на cell
    std::unique_ptr<Cell> p_new_cell = std::make_unique<Cell>(*this, pos);
    // создаём новую ячейку
    Cell* new_cell = static_cast<Cell*>(p_new_cell.get());
    // устанавливаем значение ячейки
    (*new_cell).Set(text);
    // проверяем на циклические зависимости новое содержимое cell
    // (проверка на создание новой ячейки, которая напрямую ссылается сама на себя)
    std::vector<Position> ref_cells = (*new_cell).GetReferencedCells();
    auto it = std::find(ref_cells.begin(), ref_cells.end(), pos);
    if (it != ref_cells.end()) {
        throw CircularDependencyException("Circular dependency detected!");
    }
    return p_new_cell;
}

void Sheet::UpdatesReferences(Cell* new_cell) {
    // обнавляем cсылки
    // вниз
    for (Position pos_ref : (*new_cell).GetReferencedCells()) {
        Cell* ref_cell = PositionToCell(pos_ref);
        // если ячейки нет
        if (ref_cell == nullptr) {
            // добавляем в таблицу пустую ячейку
            ref_cell = AddEmptyCell(pos_ref);
        }
        PositionToCell(pos_ref)->GetGraphReference().AddDependency(new_cell);
        new_cell->UpdateGraphReference();
    }

    // вверх
    for (CellInterface* cell_dep : new_cell->GetGraphReference().GetDependent()) {
        // добавляем ссылук на новую ячейку
        static_cast<Cell*>(cell_dep)->GetGraphReference().AddReferences(new_cell);
    }
}
