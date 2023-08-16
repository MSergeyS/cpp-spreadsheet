#pragma once

#include <optional>

#include "common.h"
#include "formula.h"
#include <unordered_map>
#include <unordered_set>

// ячейка с данными
// класс-обёртку Cell над наслдедниками метода Imp 
class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet, const Position& position);
    ~Cell() override;
    void Set(std::string text);
    void Clear();
    Value GetValue() const override;
    std::string GetText() const override;
    Position GetPosition() const;
    std::vector<Position> GetReferencedCells() const override;

    // Метод проверяет кэшированы ли данные в ячейке
    bool IsCacheValid() const;
    // Метод сбрасывает содержимое кэша ячейки
    void InvalidateCache();


    class GraphReference {
    public:
        GraphReference() = default;
        ~GraphReference() = default;

        // работа с ячейками, которые ссылаются (зависящие)
        void AddDependency(CellInterface* cell);
        void DeleteDependency(CellInterface* cell);
        const std::vector<Cell*> GetDependent() const;

        // работа с ячейками, на которые ссылается (задающие)
        void AddReferences(CellInterface* cell);
        void DeleteReferences(CellInterface* cell);
        const std::vector<Cell*> GetReferences() const;
        void UpdateReferences(std::vector<Cell*> cells_referenced);

        // проверяет циклическую зависимость start_cell_ptr от списка ссылок cells_referenced
        bool IsCyclicDependent(const Cell* start_cell,
                    std::vector<Cell*> cells_referenced) const;

        // сбрасывает содержимое кэша зависящих ячеек
        void InvalidateCacheDependent();

    private:
        // указатели на ячейки, на которые ссылается ячейка
        std::unordered_set<CellInterface*> cells_referenced_ = {};
        // указатели на ячейки, которые ссылаются на ячейку
        std::unordered_set<CellInterface*> cells_dependent_ = {};
    };

    // граф связей ячеек в таблице
    GraphReference& GetGraphReference();
    void UpdateGraphReference();

private:  
    class Impl;  // Forward declaration

    std::unique_ptr<Impl> impl_;

    SheetInterface* sheet_ = nullptr;
    Position position_ = Position::NONE;
    GraphReference graph_reference_;

    // базовый класс Impl для ячеек разных типов
    class Impl {
    public:
        virtual std::string GetText() const = 0;
        virtual Value GetValue() = 0;
        virtual ~Impl() = default;

        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual void InvalidateCache() = 0;     // Инвалидация кэша
        virtual bool IsCached() const = 0;        // Проверка валидности кэша

    protected:
        Impl() = default;
    };

    // пустая ячейка
    class EmptyImpl final : public Impl {
    public:
        EmptyImpl() = default;
        Value GetValue() override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        void InvalidateCache() override;
        bool IsCached() const override;
    };

    // текстовая ячейка
    class TextImpl final : public Impl {
    public:
        TextImpl(const std::string& text);
        CellInterface::Value GetValue() override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        void InvalidateCache() override;
        bool IsCached() const override;
    private:
        std::string text_;
    };

    // формульная ячейка 
    class FormulaImpl final : public Impl {
    public:
        FormulaImpl(const std::string& text, SheetInterface& sheet);
        void Set(std::string text);
        CellInterface::Value GetValue() override;
        std::string GetText() const override;
        void ClearCahce();
        std::vector<Position> GetReferencedCells() const override;
        void InvalidateCache() override;
        bool IsCached() const override;
    private:
        std::unique_ptr<FormulaInterface> formula_;
        std::optional<CellInterface::Value> cache_value_;
        SheetInterface* sheet_;
    };

};  //class Cell 
