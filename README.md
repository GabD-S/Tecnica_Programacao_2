TP2 — Sistema de Backup
Guia de Construção (apenas build)

Este README descreve somente como construir, testar e executar as ferramentas de qualidade do projeto. Não contém detalhes funcionais do sistema.

Requisitos
- Linux com make e g++ (C++17)
- Python 3 (para executar cpplint.py)
- cppcheck (análise estática)
- valgrind (verificação de memória)
- lcov e genhtml (cobertura)
- doxygen (documentação)

Estrutura do projeto
- src/: código-fonte C++
- include/: headers públicos
- tests/: testes (Catch2 single-header já incluso em catch.hpp)
- bin/: binários gerados (tests)
- build/: objetos, relatórios e cobertura

Comandos principais (Makefile)
- Compilar e executar testes
	make test

- Lint (cpplint)
	make lint

- Análise estática (cppcheck)
	make static
	Resultado em build/cppcheck-report.txt

- Memcheck (valgrind)
	make memcheck

- Cobertura (gcov/lcov/genhtml)
	make coverage
	Relatório HTML em build/coverage_html/index.html

- Documentação (Doxygen)
	make doc
	Saída em docs/

- Limpeza
	make clean

Detalhes de compilação
- Padrão: g++ -std=c++17 -Wall -Wextra -pedantic
- Includes: -Iinclude -I.
- O alvo de testes liga diretamente os arquivos de tests/ ao lado dos objetos de src/.

Personalização
- É possível estender CXXFLAGS ao invocar o make:
	make CXXFLAGS="-std=c++17 -Wall -Wextra -O0 -g -Iinclude -I."

Observações
- O projeto usa Catch2 (single header) já presente no repositório (catch.hpp), sem dependências externas adicionais de teste.
