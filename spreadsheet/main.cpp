#include "common.h"
#include "test_runner_p.h"

#include "FormulaAST.h"
#include "formula.h"

inline std::ostream& operator<<(std::ostream& output, Position pos) {
    return output << "(" << pos.row << ", " << pos.col << ")";
}

inline Position operator"" _pos(const char* str, std::size_t) {
    return Position::FromString(str);
}

inline std::ostream& operator<<(std::ostream& output, Size size) {
    return output << "(" << size.rows << ", " << size.cols << ")";
}

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit(
        [&](const auto& x) {
            output << x;
        },
        value);
    return output;
}

inline std::ostream& operator<<(std::ostream& output, const std::vector<Position>& cells)
{
    output << "(";
    bool is_first = true;
    for (auto& cell : cells)
    {
        if (is_first) {
            is_first = false;
        }
        else {
            output << ", ";
        }
        output << cell.ToString();
    }
    output << ")" << std::endl;
    return output;
}

namespace {

    void TestEmpty() {
        auto sheet = CreateSheet();
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 0, 0 }));
    }

    void TestInvalidPosition() {
        auto sheet = CreateSheet();
        try {
            sheet->SetCell(Position{ -1, 0 }, "");
        }
        catch (const InvalidPositionException&) {
        }
        try {
            sheet->GetCell(Position{ 0, -2 });
        }
        catch (const InvalidPositionException&) {
        }
        try {
            sheet->ClearCell(Position{ Position::MAX_ROWS, 0 });
        }
        catch (const InvalidPositionException&) {
        }
    }

    void TestSetCellPlainText() {
        auto sheet = CreateSheet();

        auto checkCell = [&](Position pos, std::string text) {

            //std::cout << pos << " = " << text << std::endl;

            sheet->SetCell(pos, text);
            CellInterface* cell = sheet->GetCell(pos);
            ASSERT(cell != nullptr);
            ASSERT_EQUAL(cell->GetText(), text);
            ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), text);
        };

        checkCell("A1"_pos, "Hello");
        checkCell("A1"_pos, "World");
        checkCell("B2"_pos, "Purr");
        checkCell("A3"_pos, "Meow");

        const SheetInterface& constSheet = *sheet;
        ASSERT_EQUAL(constSheet.GetCell("B2"_pos)->GetText(), "Purr");

        sheet->SetCell("A3"_pos, "'=escaped");
        CellInterface* cell = sheet->GetCell("A3"_pos);
        ASSERT_EQUAL(cell->GetText(), "'=escaped");
        ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), "=escaped");        
    }

    void TestClearCell() {
        auto sheet = CreateSheet();

        sheet->SetCell("C2"_pos, "Me gusta");
        sheet->ClearCell("C2"_pos);
        ASSERT(sheet->GetCell("C2"_pos) == nullptr);
        
        sheet->ClearCell("A1"_pos);
        sheet->ClearCell("J10"_pos);
    }

    void TestPrint() {
        auto sheet = CreateSheet();
        sheet->SetCell("A2"_pos, "meow");
        sheet->SetCell("B2"_pos, "=1+2");
        sheet->SetCell("A1"_pos, "=1/0");

        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 2 }));

        std::ostringstream texts;
        sheet->PrintTexts(texts);
        //std::cout << texts.str() << std::endl;
        ASSERT_EQUAL(texts.str(), "=1/0\t\nmeow\t=1+2\n");

        std::ostringstream values;
        sheet->PrintValues(values);
        //std::cout << values.str() << std::endl;
        std::string out = values.str();
        ASSERT_EQUAL(values.str(), "#DIV/0!\t\nmeow\t3\n");

        sheet->ClearCell("B2"_pos);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 1 }));
    }

}  // namespace

void TestCellExpr() {
    auto sheet = CreateSheet();

    sheet->SetCell("C2"_pos, "Me gusta");
    sheet->SetCell("C3"_pos, "=1+2");
    sheet->SetCell("A3"_pos, "4.017");
    sheet->SetCell("B1"_pos, "=1+A3+14-C3");
    sheet->SetCell("D6"_pos, "4.056");
    sheet->SetCell("B7"_pos, "=1-B1+14-C3");
    sheet->SetCell("C7"_pos, "=(1-B1+14-C3)/D3");
    sheet->SetCell("D3"_pos, "0");
    sheet->SetCell("C5"_pos, "=(1-B1+14-C3)/C2");
    sheet->SetCell("A1"_pos, "=1/0");
    sheet->SetCell("C1"_pos, "4.1t2d/");
    sheet->SetCell("C4"_pos, "=C1");
    sheet->SetCell("D5"_pos, "=D6");
    sheet->SetCell("K5"_pos, "=N5");
    sheet->SetCell("N5"_pos, "758");

    //auto cells = sheet->GetCell("K5"_pos)->GetReferencedCells();
    //std::cout << cells;
    //ASSERT_EQUAL(cells, (std::vector<Position>{ "C3"_pos, "A3"_pos }));

    auto B1_text = sheet->GetCell("B1"_pos)->GetText();
    auto B1_value = sheet->GetCell("B1"_pos)->GetValue();
    auto B7_text = sheet->GetCell("B7"_pos)->GetText();
    //std::cout << B7_text << std::endl;
    auto B7_value = sheet->GetCell("B7"_pos)->GetValue();
    //std::cout << B7_value << std::endl;

    auto C7_text = sheet->GetCell("C7"_pos)->GetText();
    //std::cout << C7_text << std::endl;
    auto C7_value = sheet->GetCell("C7"_pos)->GetValue();
    //std::cout << C7_value << std::endl;

    auto C5_text = sheet->GetCell("C5"_pos)->GetText();
    //std::cout << C5_text << std::endl;
    auto C5_value = sheet->GetCell("C5"_pos)->GetValue();
    //std::cout << C5_value << std::endl;

    auto A1_value = sheet->GetCell("A1"_pos)->GetValue();
    //std::cout << A1_value << std::endl;

    auto D6_value = sheet->GetCell("D6"_pos)->GetValue();
    //std::cout << D6_value << std::endl;

    auto C4_value = sheet->GetCell("C4"_pos)->GetValue();
    //std::cout << C4_value << std::endl;

    auto D5_value = sheet->GetCell("D5"_pos)->GetValue();
    //std::cout << D5_value << std::endl;

    try {
        sheet->SetCell("N5"_pos, "=K5");
        assert(false);
    }
    catch (...) {

    }
    auto N5_value = sheet->GetCell("N5"_pos)->GetValue();
    //std::cout << N5_value << std::endl;
}

void TestRef() {

    using namespace std::literals;
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "=(1+2)*3"); // 9
    sheet->SetCell("B1"_pos, "=1+(2*3)"); // 7

    sheet->SetCell("A2"_pos, "some"); //

    sheet->SetCell("B2"_pos, "=A1/B5"); // #DIV0
    sheet->SetCell("C1"_pos, "=ZZZZ20000+5"); // #REF

    sheet->SetCell("C2"_pos, "=A2/A1"); // #VALUE

    sheet->SetCell("C3"_pos, "=A1+B1"); // 16
    sheet->SetCell("D3"_pos, "=A1+B1+C3"); //32

    sheet->SetCell("B5"_pos, "=1/0");

    auto refs_c3 = sheet->GetCell("C3"_pos)->GetReferencedCells();
    auto refs_d3 = sheet->GetCell("D3"_pos)->GetReferencedCells();
    assert(refs_c3.size() == 2);
    assert(refs_d3.size() == 3);

    assert(std::get<double>(sheet->GetCell("C3"_pos)->GetValue()) == 16);
    assert(std::get<double>(sheet->GetCell("D3"_pos)->GetValue()) == 32);

    auto tets = sheet->GetCell("C3"_pos)->GetValue();

    assert(std::get<FormulaError>(sheet->GetCell("C2"_pos)->GetValue()) == FormulaError::Category::Value);
    assert(std::get<FormulaError>(sheet->GetCell("B2"_pos)->GetValue()) == FormulaError::Category::Div0);
    assert(std::get<FormulaError>(sheet->GetCell("C1"_pos)->GetValue()) == FormulaError::Category::Ref);
}

void TestCache() {
    auto sheet = CreateSheet();

    sheet->SetCell("C2"_pos, "Me gusta");
    sheet->SetCell("C3"_pos, "=1+2");
    sheet->SetCell("A3"_pos, "4.017");
    sheet->SetCell("B1"_pos, "=1+A3+14-C3");
    sheet->SetCell("D6"_pos, "4.056");
    sheet->SetCell("B7"_pos, "=1-B1+14-C3");
    sheet->SetCell("C7"_pos, "=(1-B1+14-C3)/D3");
    sheet->SetCell("D3"_pos, "0");
    sheet->SetCell("C5"_pos, "=(1-B1+14-C3)/C2");
    sheet->SetCell("A1"_pos, "=1/0");
    sheet->SetCell("C1"_pos, "4.1t2d/");
    sheet->SetCell("C4"_pos, "=C1");
    sheet->SetCell("D5"_pos, "=D6");
    sheet->SetCell("K5"_pos, "=N5");
    sheet->SetCell("N5"_pos, "758");

    auto B1_value = sheet->GetCell("B1"_pos)->GetValue();
    auto B7_value = sheet->GetCell("B7"_pos)->GetValue();
    auto C7_value = sheet->GetCell("C7"_pos)->GetValue();
    auto C5_value = sheet->GetCell("C5"_pos)->GetValue();
    auto A1_value = sheet->GetCell("A1"_pos)->GetValue();
    auto D6_value = sheet->GetCell("D6"_pos)->GetValue();
    auto C4_value = sheet->GetCell("C4"_pos)->GetValue();
    auto D5_value = sheet->GetCell("D5"_pos)->GetValue();

    B1_value = sheet->GetCell("B1"_pos)->GetValue();
    B7_value = sheet->GetCell("B7"_pos)->GetValue();
    C7_value = sheet->GetCell("C7"_pos)->GetValue();
    C5_value = sheet->GetCell("C5"_pos)->GetValue();
    A1_value = sheet->GetCell("A1"_pos)->GetValue();
    D6_value = sheet->GetCell("D6"_pos)->GetValue();
    C4_value = sheet->GetCell("C4"_pos)->GetValue();
    D5_value = sheet->GetCell("D5"_pos)->GetValue();

    sheet->SetCell("A3"_pos, "4.017");

    B1_value = sheet->GetCell("B1"_pos)->GetValue();
}

void Test() {
    auto sheet = CreateSheet();
    sheet->SetCell("A1"_pos, "=A2 + A3 + A4");
    sheet->SetCell("A2"_pos, "=A3");
    //sheet->SetCell("A3"_pos, "=5");
    //sheet->SetCell("A4"_pos, "=A3");

    std::ostringstream texts;
    sheet->PrintTexts(texts);
    std::cout << texts.str() << std::endl;
//    ASSERT_EQUAL(texts.str(), "=2+2\n");

    std::ostringstream values;
    sheet->PrintValues(values);
    std::cout << values.str() << std::endl;
    std::string out = values.str();
    //ASSERT_EQUAL(values.str(), "15\n");

    
}

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestEmpty);
    RUN_TEST(tr, TestInvalidPosition);
    RUN_TEST(tr, TestSetCellPlainText);
    RUN_TEST(tr, TestClearCell);
    RUN_TEST(tr, TestPrint);
    RUN_TEST(tr, TestCellExpr);
    RUN_TEST(tr, TestRef);
    RUN_TEST(tr, TestCache);
    RUN_TEST(tr, Test);
    return 0;
}
