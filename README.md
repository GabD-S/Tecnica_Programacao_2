TP2 — Sistema de Backup e Restore (C++)

[![CI](https://github.com/GabD-S/Tecnica_Programacao_2/actions/workflows/ci.yml/badge.svg)](https://github.com/GabD-S/Tecnica_Programacao_2/actions/workflows/ci.yml)

Sistema de sincronização entre HD e PEN com interface de linha de comando (CLI), testes automatizados (Catch2), análise estática e cobertura. Documentação clara para executar, testar e manter.

Sumário
- Visão geral
- Pré‑requisitos
- Guia rápido (3 comandos)
- Uso do CLI (com exemplos)
- Formato do Backup.parm
- Códigos de retorno
- Estrutura do projeto
- Comandos do Makefile (build, testes, lint, estática, cobertura, doc)
- Integração Contínua (GitHub Actions)
- Troubleshooting

Visão geral
- Backup: copia/atualiza do HD para o PEN quando o HD é mais novo.
- Restore: copia/atualiza do PEN para o HD quando o PEN é mais novo.
- Diretórios listados no parm não implicam recursão automática.
- Erros são acumulados: código 5 (falha de escrita) tem precedência sobre 4 (arquivos faltando).
- Timestamps de modificação são preservados nas cópias.

Pré‑requisitos (Linux)
- make, g++ (C++17)
- Python 3 (para lint opcional via cpplint)
- cppcheck (análise estática), valgrind (memória)
- lcov e genhtml (cobertura), doxygen (opcional)

Guia rápido (3 comandos)
- Executar testes (compila e roda):
```bash
make test
```
- Executar o CLI (modo backup):
```bash
make run
# ou diretamente
./bin/tp2_cli --mode backup --hd "/caminho/HD" --pen "/caminho/PEN" --parm "/caminho/Backup.parm"
```
- Cobertura:
```bash
make coverage
# Abre: build/coverage_html/index.html
```

Uso do CLI
- Binário: ./bin/tp2_cli
- Sintaxe:
```bash
tp2_cli --mode <backup|restore> --hd <path> --pen <path> [--parm <file>]
```
- Parâmetros:
  - --mode backup|restore
  - --hd <path> diretório base do HD
  - --pen <path> diretório base do PEN
  - --parm <file> arquivo de lista (default: Backup.parm)

Exemplos
- Backup (HD -> PEN):
```bash
./bin/tp2_cli --mode backup --hd "/dados/hd" --pen "/mnt/pen" --parm "/dados/Backup.parm"
```
- Restore (PEN -> HD):
```bash
./bin/tp2_cli --mode restore --hd "/dados/hd" --pen "/mnt/pen" --parm "/dados/Backup.parm"
```

Formato do Backup.parm
- Uma entrada por linha, relativa à raiz de --hd/--pen
- Espaços nas extremidades são ignorados
- Linhas em branco são ignoradas
- Linhas iniciadas por # ou ; são comentários
- Diretórios listados são ignorados (sem recursão implícita)

Exemplo:
```ini
# Lista de arquivos para sincronizar
; linhas iniciadas por ; também são comentários

Documentos/relatorio.pdf
Fotos/viagem.jpg

# linhas em branco são ignoradas
Scripts/backup.sh
```

Códigos de retorno (exit code)
- 0: sucesso
- 1: parâmetro obrigatório faltando/vazio
- 2: operação não suportada (modo inválido)
- 3: exceção/erro inesperado
- 4: entradas ausentes na fonte (missing)
- 5: falha de escrita (tem precedência sobre 4)

Estrutura do projeto
- src/: código‑fonte C++ (inclui o main do CLI)
- include/: headers públicos (API de backup/restore)
- tests/: testes (Catch2 single‑header em catch.hpp)
- bin/: binários gerados (tests, tp2_cli)
- build/: objetos, relatórios e cobertura
- examples/: exemplos (Backup.parm)

Comandos do Makefile
- Build e testes:
```bash
make all     # compila (gera bin/tests)
make test    # compila e roda testes
```
- Executar CLI:
```bash
make run     # executa ./bin/tp2_cli
make app     # apenas constrói o CLI
```
- Lint e estática:
```bash
make lint    # cpplint (prefere o do sistema; fallback ao script)
make static  # cppcheck (relatório em build/cppcheck-report.txt)
```
- Memória e cobertura:
```bash
make memcheck   # valgrind
make coverage   # lcov/genhtml -> build/coverage_html/
```
- Documentação e limpeza:
```bash
make doc     # doxygen (se Doxyfile configurado)
make clean   # remove bin/build/artefatos de cobertura
```

Integração Contínua (CI)
- GitHub Actions: workflow CI roda em pushes/PRs para main e develop.
- Etapas: build/test, lint, análise estática, cobertura (artefatos publicados).
- Badge no topo deste README mostra o status atual.

Troubleshooting
- Caminhos com espaços: sempre use aspas em --hd, --pen e --parm.
- Permissões no pendrive: se o retorno for 5 (falha de escrita), verifique permissões e montagem.
- CLI não encontrado (exit 127): certifique‑se de que o binário foi gerado (make app ou make test) e está em ./bin/tp2_cli.
- cpplint “barulhento”: no CI é instalado via pip (preferível); localmente pode usar o script embarcado.
