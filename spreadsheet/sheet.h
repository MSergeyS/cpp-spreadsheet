#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <iostream>

class Sheet : public SheetInterface {
public:
    Sheet() = default;
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(const Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Производит сброс кэша для указанной ячейки и всех зависящих от нее
    void InvalidateCell(const Position& pos);

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;

    // Единый для всй таблицы словарь зависимых ячеек (ячейка - список зависимых от нее)
    std::unordered_map<CellInterface*, std::unordered_set<CellInterface*>> cells_dependent_;

    int max_row_ = 0;    // Число строк в Printable Area
    int max_col_ = 0;    // Число столбцов в Printable Area

    void ResizeSheet(const Position pos);
    bool CellExists(Position pos) const;
    void UpdatePrintableSize();

    //std::string, double, FormulaError
	struct OstreamSolutionPrinter {
		std::ostream &out;

		void operator()(std::string str) const {
			out << str;
		}
		void operator()(double value) const {
			out << value;
		}
		void operator()(FormulaError er) const {
			out << er;
		}
	};

    Cell* PositionToCell(Position pos) const;
    std::unique_ptr<Cell> PreCreateNewCell(const Position pos, const std::string text);
    Cell* AddEmptyCell(const Position pos);
    void UpdatesReferences(Cell* new_cell);
};
