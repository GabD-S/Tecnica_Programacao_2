TP2 — Sistema de Backup

Este projeto implementa um sistema de Backup/Restore com interface de linha de comando (CLI) e testes automatizados (Catch2).

Requisitos
- Linux com make e g++ (C++17)
- Python 3 (para executar cpplint.py)
- cppcheck (análise estática)
- valgrind (verificação de memória)
- lcov e genhtml (cobertura)
- doxygen (documentação)

Estrutura do projeto
- src/: código-fonte C++ (inclui o main do CLI)
- include/: headers públicos (API de backup/restore)
- tests/: testes (Catch2 single-header já incluso em catch.hpp)
- bin/: binários gerados (tests, tp2_cli)
- build/: objetos, relatórios e cobertura
- explicacao/: materiais e roteiros

Como compilar e testar
- Executar testes (compila tudo e roda):
	make test

- Executar apenas a compilação dos binários:
	make all

- Executar a aplicação CLI:
	make run
	# ou diretamente
	./bin/tp2_cli --mode backup --hd "/caminho/HD" --pen "/caminho/PEN" --parm "/caminho/Backup.parm"

Uso do CLI
Binário: ./bin/tp2_cli

Parâmetros:
- --mode backup|restore
- --hd <path> diretório do HD
- --pen <path> diretório do PEN
- --parm <file> arquivo de lista (padrão: Backup.parm)

Exemplos:
- Backup (HD -> PEN):
	./bin/tp2_cli --mode backup --hd "/dados/hd" --pen "/mnt/pen" --parm "/dados/Backup.parm"

- Restore (PEN -> HD):
	./bin/tp2_cli --mode restore --hd "/dados/hd" --pen "/mnt/pen" --parm "/dados/Backup.parm"

Códigos de retorno (exit code) e mensagens
- 0: sucesso
- 1: parâmetro obrigatório faltando/vazio
- 2: operação não suportada (modo inválido)
- 3: exceção/erro inesperado
- 4: entradas ausentes na fonte (missing)
- 5: falha de escrita (tem precedência sobre 4)

Formato do Backup.parm
- Uma entrada por linha, relativa à raiz informada por --hd/--pen
- Espaços em branco nas extremidades são ignorados (trim)
- Linhas em branco são ignoradas
- Linhas iniciadas por # ou ; são comentários e são ignoradas
- Diretórios listados não implicam recursão automática (sem recursão implícita)

Decisões de comportamento (resumo)
- Sem recursão implícita para diretórios no parm
- Continuidade mesmo com múltiplos erros; código 5 tem precedência sobre 4
- Preservação de timestamps após cópia/atualização

Comandos principais (Makefile)
- Testes:
	make test

- Lint (cpplint):
	make lint

- Análise estática (cppcheck):
	make static
	# Relatório em build/cppcheck-report.txt

- Memcheck (valgrind):
	make memcheck

- Cobertura (gcov/lcov/genhtml):
	make coverage
	# Relatório HTML em build/coverage_html/index.html

- Documentação (Doxygen):
	make doc

- Limpeza:
	make clean

Detalhes de compilação
- g++ -std=c++17 -Wall -Wextra -pedantic, includes em -Iinclude -I.
- O alvo de testes linka objetos de src/ com os arquivos de tests/ (Catch2 fornece o main de teste).

Personalização
- É possível estender CXXFLAGS ao invocar o make:
	make CXXFLAGS="-std=c++17 -Wall -Wextra -O0 -g -Iinclude -I."

Exemplo de Backup.parm
```
# Lista de arquivos para sincronizar
; linhas iniciadas por ; também são comentários

Documentos/relatorio.pdf
Fotos/viagem.jpg

# linhas em branco são ignoradas
Scripts/backup.sh
```

Observações
- O projeto usa Catch2 (single header) já presente no repositório (catch.hpp).
