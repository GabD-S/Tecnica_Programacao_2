#pragma once
#include <string>
#include <vector>

namespace tp2 {

/** \brief Operações suportadas pelo sistema de sincronização. */
enum class Operation { Backup, ///< Copia/atualiza de HD para PEN
                       Restore ///< Copia/atualiza de PEN para HD
};

/** \brief Resultado de uma ação de sincronização.
 *  \details Códigos de retorno:
 *  - 0: sucesso
 *  - 1: parâmetro obrigatório faltando/vazio
 *  - 2: operação não suportada
 *  - 3: exceção/erro inesperado
 *  - 4: entradas ausentes na fonte (missing)
 *  - 5: falha de escrita (tem precedência sobre 4)
 */

struct ActionResult {
    int code;              ///< 0 sucesso; >0 conforme tabela acima
    std::string message;   ///< mensagem opcional de detalhe
};

/** \brief Executa a sincronização conforme o modo e a lista do arquivo parm.
 *  \param hdPath Caminho base do HD
 *  \param penPath Caminho base do PEN
 *  \param paramFile Caminho para o arquivo de parâmetros (ex.: Backup.parm)
 *  \param op Modo de operação (Backup ou Restore)
 *  \return ActionResult com código e mensagem
 *  \note O arquivo de parâmetros aceita:
 *    - Uma entrada por linha (relativa ao diretório base)
 *    - Linhas em branco e espaços em branco nas extremidades são ignorados
 *    - Linhas iniciadas por # ou ; são comentários
 *    - Diretórios listados não implicam recursão automática
 */
ActionResult execute_backup(const std::string& hdPath,
                           const std::string& penPath,
                           const std::string& paramFile,
                           Operation op);

/** \brief Lê a lista de entradas do arquivo de parâmetros.
 *  \param paramFile Caminho do arquivo de parâmetros
 *  \return Vetor de strings com as entradas normalizadas
 */
std::vector<std::string> read_param_list(const std::string& paramFile);

} // namespace tp2
