#pragma once

#include <optional>

#include "common.h"
#include "formula.h"

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
    std::vector<Position> GetReferencedCells() const override;

private:

    class Impl;  // Forward declaration

    std::unique_ptr<Impl> impl_;

    SheetInterface* sheet_ = nullptr;
    Position position_ = Position::NONE;

    // базовый класс Impl для ячеек разных типов
    class Impl {
    public:
        virtual std::string GetText() const = 0;
        virtual Value GetValue() const = 0;
        virtual ~Impl() = default;
        virtual std::vector<Position> GetReferencedCells() const = 0;
    protected:
        Impl() = default;
    };

    // пустая ячейка
    class EmptyImpl final : public Impl {
    public:
        EmptyImpl() = default;
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    };

    // текстовая ячейка
    class TextImpl final : public Impl {
    public:
        TextImpl(const std::string& text);
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    private:
        std::string text_;
    };

    // формульная ячейка 
    class FormulaImpl final : public Impl {
    public:
        FormulaImpl(const std::string& text, SheetInterface& sheet);
        void Set(std::string text);
        CellInterface::Value GetValue() const override;
        std::string GetText() const override;
        void ClearCahce();
        std::vector<Position> GetReferencedCells() const override;
    private:
        std::unique_ptr<FormulaInterface> formula_;
        std::optional<FormulaInterface::Value> cache_;
        SheetInterface* sheet_;
    };

};  //class Cell 