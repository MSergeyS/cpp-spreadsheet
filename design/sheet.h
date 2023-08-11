#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;

    class Graph {
    public:
        // рёбра графа (связи)
        struct Node {
            // указатели на ячейки, на которые ссылается
            std::unordered_set<CellInterface*> referenced_cells_to;
            // указатели на ячейки, которые ссылаются
            std::unordered_set<CellInterface*> referenced_cells_from;
        };

        Graph() = default;
        ~Graph() = default;

        // эти методы возвращают контейнер с ячейками кэш, которых стал невалидным
        // (ячейки которые ссылались на изменяемую ячейку напрямую или опоредовано)
        // добавляем новый узел
        std::unordered_set<const CellInterface*> AddCell(const CellInterface* p_cell,
                             const std::vector<const CellInterface*> referenced_cells_to);
        // изменяем узел
        std::unordered_set<const CellInterface*> ChangeCell(const CellInterface* p_cell_old,
                             const CellInterface* p_cell_new,
                             const std::vector<const CellInterface*> referenced_cells_to);
        // удаляем узел
        std::unordered_set<const CellInterface*> DeleteCell(const CellInterface* p_cell);


        // проверка на циклические зависимости
        bool IsCyclicReference(const CellInterface* p_cell,
                             const std::vector<const CellInterface*> referenced_cells_to) const;

    private:
        // граф - узел (казатель на ячейку)
        std::unordered_map<CellInterface*, Node> graph_;
    };

    Graph graph_;

    int max_row_ = 0;    // Число строк в Printable Area
    int max_col_ = 0;    // Число столбцов в Printable Area

    void CreateCell(const Position pos);
    bool CellExists(Position pos) const;
    void UpdatePrintableSize();

    //std::string, double, FormulaError
	struct OstreamSolutionPrinter {
		std::ostream &out;

		void operator()(std::string str) const {
			out << str;
		}
		void operator()(double val) const {
			out << std::to_string(val);
		}
		void operator()(FormulaError er) const {
			out << er;
		}
	};

};
