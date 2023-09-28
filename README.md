# Spreadsheet
### __*Дипломный проект: Электронная таблица*__
  *SpreadSheet - упрощённый аналог существующих решений: лист таблицы Microsoft Excel или Google Sheets. Поддерживает текстовые и числовые ячейки, основные арифметические операции, а также ссылки на другие ячейки.*

  *Проект курса Яндекс Практикума "Разработчик С++"*

  ---
  ## Основные функции:
- ввод текстовых и числовых данных в двумерный массив - задание значений ячеек таблицы
- выполнение основных арифметических операций над числовыми значениями ячеек таблицы
- поддерживает ссылки на другие ячейки
- обеспечивается корректность, устойчивость работы, а также согласованость всех компонентов таблицы (защита от некорректного ввода данных)
- поддерживается кеширование данных ячеек

---
## Общее описание
### Ячейки и индексы
Таблица хранит в себе ячейки ```Cell```. __Для пользователя__ ячейка таблицы задаётся своим индексом, то есть строкой вида ```А1```, ```С14``` или ```RD2```. Причём ячейка с индексом ```А1``` — это ячейка в левом верхнем углу листа. Количество строк и столбцов в таблице не превышает ```16384```. То есть предельная позиция ячейки равна ```(16383, 16383)``` с индексом ```XFD16384```. Если позиция ячейки выходит за эти границы, то ячейка невалидна по определению. Структура позиции определена в файле ```common.h``` и содержит поля ```col``` и ```row``` – индексы строк и столбцов, используемые для доступа к ячейкам листа.
__В программе__ положение ячейки описывается позицией, то есть номерами её строки и столбца, начиная от 0, как это принято в С++. Например, индексу “А1” соответствует позиция (0, 0), а индексу “AB15” — позиция (14, 27).
Индекс ячейки для пользователя состоит из двух частей:
- строка из заглавных букв латинского алфавита, обозначающая столбец;
- число, обозначающее порядковый номер строки.

На пересечении столбца и строки и находится ячейка. Для пользователя строки нумеруются с __*1*__, в программном представлении — с __*0*__.
Для конвертации координаты ячейки из программной позиции в её пользовательский индекс в таблице и обратно используются функции ```Position::FromString()``` и ```Position::ToString()``` сщщтветственно.

### Минимальная печатная область
Чтобы напечатать таблицу, нужно знать размер минимальной печатной области. Это минимальная прямоугольная область с вершиной в ячейке A1, содержащая все непустые ячейки. Структура ```Size``` определена в файле ```common.h```. Она содержит количество строк и столбцов в минимальной печатной области.

### Методы, обращающиеся к ячейке по индексу
- __```SetCell(Position, std::string)```__ – задаёт содержимое ячейки по индексу Position. Если ячейка пуста – она создаётся. Нужно задать ячейке текст методом ```Cell::Set(std::string)```;
 - __```Cell* GetCell(Position pos)```__ – константный и неконстантный геттеры, которые возвращают указатель на ячейку, расположенную по индексу ```pos```. Если ячейка пуста, возвращают nullptr;
- __```void ClearCell(Position pos)```__ – очищает ячейку по индексу. Последующий вызов ```GetCell()``` для этой ячейки вернёт ```nullptr```. При этом может измениться размер минимальной печатной области.

### Методы, применимые к таблице целиком
- __```Size GetPrintableSize()```__ – определяет размер минимальной печатной области. Специально для него в файле ```common.h``` определена структура ```Size```. Она содержит количество строк и столбцов в минимальной печатной области;
- __```void PrintText(std::ostream&)```__ – выводит текстовые представления ячеек:
    * [x] для *текстовых ячеек* – это текст, который пользователь задал в методе ```Set()```, то есть не очищенный от ведущих апострофов ```'```;
    * [x] для *формульных ячеек* – это формула, очищенная от лишних скобок, как Formula::GetExpression(), но с ведущим знаком ```=```.
- __```void PrintValues(std::ostream&)```__ – выводит значения ячеек — строки, числа или ```FormulaError```, — как это определено в ```Cells::GetValue()```.

### Вычисление значений в ячейках
Допустим, в ячейке __*А3*__ находится формула ```=1+2*7```. Её легко вычислить: это ```15```. В ячейке __*A2*__ находится текст ```3```. Формально ячейка не формульная. Но её текст можно интерпретировать как число. Поэтому её значение – ```3```. В ячейке __*С2*__ записана формула ```=A3/A2```. Чтобы её вычислить, надо разделить значение ячейки А3 на значение ячейки __*А2*__. Результат – __*15/3 = 5*__. Если формула содержит индекс пустой ячейки, то значение пустой ячейки — __*0*__.

Для реализации последовательности действий используемой при вычислении очень подходит структура данных __дерево__. В компьютерах формулы представляются именно в виде деревьев.

#### Дерево
__Дерево__ — это связный граф без циклов. У каждого узла, кроме одного, есть один узел-родитель и несколько узлов-потомков. В нашем случае не больше двух: правый и левый.
- Узлы без потомков называются листья.
- Узлы, у которых есть потомки, называются внутренние вершины.
- Узел, у которого нет родителей, называется корень.
Корень задаёт всё дерево, потому что из корня можно обойти все узлы.
В дереве проекта во внутренних узлах находятся операции, а в листьях — операнды. 
Дерево используют, потому что по нему удобно рекурсивно вычислять значение формулы. Используется __[абстрактное синтаксическое дерево (Abstract Syntax Tree, AST)](https://ps-group.github.io/compilers/ast)__.

Для разбивкb строки на токены и составлением дерева разбора есть готовое решение — __[ANTLR](https://www.antlr.org/)__. 
__ANTLR__ — это специальная программа, которая сгенерирует для нас код лексического и синтаксического анализаторов, а также код для обхода дерева разбора на С++.

### Возможные ошибки и исключения

__Ошибки вычисления__
- __Деление на 0.__ В вычислениях могут возникнуть ошибки. Например, уже известная вам ошибка «деление на 0». Если делитель равен 0, значение ячейки — ошибка ```FormulaError``` типа __```#DIV/0!```__
- __Ошибка в значении.__ Если ячейку, чей индекс входит в формулу, нельзя проинтерпретировать как число, возникает ошибка нового типа: ```FormulaError``` — нет значения __```#VALUE!```__ В следующем примере в ячейке А2 находится текст, поэтому вычисление формулы в ячейке ```С2 (=А3/А2)``` вернёт эту ошибку.

![#VALUE!](/spreadsheet/VALUE.png)

- __Ошибка в ссылке.__
Формула может содержать ссылку на ячейку, которая выходит за границы возможного размера таблицы, например ```С2 (=А1234567+ZZZZ1)```. Такая формула может быть создана, но не может быть вычислена, поэтому её вычисление вернёт ошибку __```#REF!```__.
Ошибки распространяются вверх по зависимостям. Если формула зависит от нескольких ячеек, каждая из которых содержит ошибку вычисления, результирующая ошибка может соответствовать любой из них.

Таблица должна работать корректно и устойчиво, а все компоненты должны быть согласованы. 
Если пользователь вызывает методы с некорректными аргументами, программа не должна менять таблицу, но должна кидать исключения. По ним пользователь может понять, что он сделал что-то не так.
- __Некорректная формула.__ Если в ячейку методом ```Sheet::SetCell()``` пытаются записать синтаксически некорректную формулу, например ```=A1+*```, программа выбрасывает исключение ```FormulaException```, а значение ячейки не изменится. Формула считается синтаксически некорректной, если она не удовлетворяет предоставленной грамматике.
- __Некорректная позиция.__ Программно возможно создать экземпляр класса ```Position``` c некорректной позицией, например ```(-1, -1)```. Если пользователь передаёт её в методы, программа выбрасывает исключение ```InvalidPositionException```. Методы интерфейсов — например ```Cell::GetReferencedCells()``` — всегда возвращатют корректные позиции.
- __Циклическая зависимость.__ Если пользователь пытается в методе ```Sheet::SetCell()``` записать в ячейку формулу, которая привела бы к циклической зависимости, реализация должна выбросить исключение ```CircularDependencyException```, а значение ячейки не должно измениться.

__Циклические зависимости__
Таблица должна всегда оставаться корректной. Если ячейки циклически зависят друг от друга, вычислить значения ячеек невозможно. Поэтому нельзя позволить, чтобы возникли циклические зависимости между ячейками. То есть нельзя дать пользователю задать ячейку с формулой, которая вводит циклические зависимости.  В случае обнаружение циклической зависимости будет выброшено исключение ```CircularDependencyException```, а ячейка не изменится.."
### Парсинг формул
Использована готовая грамматика и все нужные для работы ANTLR файлы. Реализована обработка формул: строятся AST формулы из собственных классов, и дальше программа работает с этим деревом.

### Кэширование данных
Чтобы не вычислять значения в ячейках лишний раз, можно хранить вычисленное значение в кэше. Например, пользователь ввёл в ячейку E3 формулу "=C4*C2". Чтобы её вычислить, нужны значения ячеек С2 и С4. Если они уже когда-то были посчитаны, можно использовать эти значения.

![cache](/spreadsheet/cache.png)

### Граф зависимостей
Для эффективной инвалидации кешированных значений можно использовать граф зависимостей. То есть для каждой ячейки нам нужно знать, от каких ячеек зависит она (исходящие ссылки), и какие ячейки зависят от неё (входящие ссылки). При изменении значения ячейки достаточно пройтись рекурсивно по всем входящим ссылкам и инвалидировать кэши соответствующих ячеек. Причём, если кеш какой-то ячейки уже был инвалидирован, нет смысла продолжать рекурсию дальше. Эта оптимизация и позволяет достичь константной сложности O(1) при повторном вызове метода ```ISheet::SetCell()``` с теми же аргументами. Кроме того, граф зависимостей упростит предотвращение циклических зависимостей. Вершинами данного графа являются ячейки, и хранить его удобно как список рёбер, входящих в и исходящих из конкретной ячейки. При изменении ячейки обновляется список исходящих ссылок (рёбер), а также списки входящих ссылок (рёбер) для всех ячеек, от которых данная ячейка зависела и станет зависеть.

![cyclic](/spreadsheet/cyclic.png)

---
## Системные требования
С++ 17
[Visual Studio](https://visualstudio.microsoft.com/ru/vs/features/cplusplus/) или компилятор C++ например [GCC(MinGW-w64) 11+](https://www.mingw-w64.org/downloads/)
[JDK – Java Development Kit](https://www.oracle.com/java/technologies/downloads/)
[Библиотека ANTLR4](https://www.antlr.org/)
[CMake version 3.8+](https://cmake.org/)

---
## Cборка и установка
1. __Устанавливаем JDK__
ANTLR написан на Java, поэтому для его работы вам понадобится комплект разработки [JDK](https://www.oracle.com/java/technologies/downloads/). Вы также можете использовать OpenJDK. Установите JDK в свою систему.
2. __Устанавливаем ANTL__
Инструкцию по установке ANTLR можно найти на сайте [antlr.org](https://www.antlr.org/). Более подробные рекомендации представлены в руководстве [Getting Started](https://github.com/antlr/antlr4/blob/master/doc/getting-started.md). Убедитесь, что JAR-файл ```antlr.jar``` находится в переменной среды ```CLASSPATH```. Это нужно для компиляции Java-кода. Если вы следовали инструкциям на сайте ANTLR, ```CLASSPATH``` уже должен быть правильным.
3. __Используем ANTLR в С++__
Чтобы ANTLR сгенерировал для нас исполняемые файлы на С++, нужно выполнить команду:
```antlr4 -Dlanguage=Cpp Formula.g4```
Рассмотрим подробнее, что сгенерировал ANTLR:
- ```Formula.interp, Formula.tokens``` — вспомогательные текстовые файлы для вашего удобства;
- ```FormulaLexer.{cpp,h}``` — код лексического анализатора;
- ```FormulaParser.{cpp,h}``` — код синтаксического анализатора;
- ```FormulaListener.{cpp,h}, FormulaBaseListener.{cpp,h}``` — код ```listener'```а, разновидности паттерна ```visitor``` для дерева разбора. Он позволит обходить дерево разбора и строить наше абстрактное синтаксическое дерево для вычисления формул.
Эти файлы будут работать в основном проекте и делать часть работы за нас.
Мы используем их результат и строим по нему абстрактное синтаксическое дерево, которое умеет вычислять формулы.
Для компиляции этого кода понадобится библиотека ```ANTLR4 C++ Runtime```. Скачайте архив ```antlr4-cpp-runtime*.zip``` из раздела ```Download``` на сайте [antlr.org](https://www.antlr.org/).
CMake-файлы для её интеграции в есть проекте. Сгенерированные файлы запишутся в папку:
 ```spreadsheet\build\antlr4cpp_generated_src\Formula```
4. __Используем ANTLR в CMake-проектах__
В папке с проектом есть файлы:
```
CMakeLists.txt
FindANTLR.cmake
```
В``` FindANTLR.cmake``` вынесены команды генерации файлов ANTLR. Сам файл подключается к ```CMakeLists.txt``` командой:
```
 include(${CMAKE_CURRENT_SOURCE_DIR}/FindANTLR.cmake)
 ```
Должна получиться следующая структура проекта:
```
spreadsheet/
├── antlr4_runtime/
│   └── Содержимое архива antlr4-cpp-runtime*.zip.
├── build/
├── antlr-4.12.0-complete.jar
├── CMakeLists.txt
├── FindANTLR.cmake
├── Formula.g4
├── Остальные файлы проекта
└── ... 
```
В папке ```antlr4_runtime``` разместите содержимое архива ```antlr4-cpp-runtime*.zip```.
В папке ```build``` выполняйте генерацию и сборку проекта.
Версия ```antlr-4.12.0-complete.jar``` может отличаться. В ```CMakeLists.txt``` замените версию JAR-файла на актуальную.

5. __Cборка программы с помощью CMake__
Собрать и установить проект (в примере сборка Debug) следующими командами:
```
cmake --build .
cmake --install .
```

---
## Запуск программы
```
spreadsheet.exe
```

---
## Используемые технологии и навыки
- C++ 17
- библиотека STL
- ООП, полиморфизм, шаблоны, лямбда-функции, стандартные алгоритмы
- абстрактное синтаксическое дерево (AST) (используется библиотекой лексического анализа [antlr4](https://www.antlr.org/))
- cmake