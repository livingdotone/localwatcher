# 📄 Documento Técnico: Arquitetura de Coleta

## 🧭 1. Visão Geral da Coleta

Este sistema monitora o comportamento do sistema local com foco em processos, uso de CPU/memória, e acesso a arquivos sensíveis, com o objetivo de gerar uma baseline de normalidade e detectar anomalias comportamentais sem uso de assinaturas.

A coleta será contínua, discreta e orientada a performance, permitindo que o sistema opere 24/7 com impacto mínimo.

## 📦 2. Itens de Monitoramento + Estratégia de Coleta

#### 🔹 Processos

| Informação            | Método/Fonte                                                    | Observações                  |
| --------------------- | --------------------------------------------------------------- | ---------------------------- |
| Nome do processo      | `Process32First/Next` via `CreateToolhelp32Snapshot`            | Útil para identificação      |
| PID                   | Mesmo acima                                                     | Necessário para rastreamento |
| Caminho do executável | `GetProcessImageFileNameW` ou `QueryFullProcessImageNameW`      | Requer `OpenProcess`         |
| Tempo de vida         | Calcular via `GetProcessTimes` + timestamp de coleta            | Pode ser estimado também     |
| Usuário (SID → nome)  | `OpenProcessToken` + `GetTokenInformation` + `LookupAccountSid` | Pode falhar sem permissões   |

#### 🔹 CPU e Memória por Processo

| Informação          | Método/Fonte                                                    | Observações                          |
| ------------------- | --------------------------------------------------------------- | ------------------------------------ |
| %CPU (por processo) | `GetProcessTimes` para processo + `GetSystemTimes` (para delta) | Mede entre dois momentos             |
| Memória atual       | `GetProcessMemoryInfo` (WorkingSetSize, PagefileUsage, etc.)    | Requires `PROCESS_QUERY_INFORMATION` |

### 🔹 Sistema de Arquivos

| Informação                            | Método/Fonte                                            | Observações                                                  |
| ------------------------------------- | ------------------------------------------------------- | ------------------------------------------------------------ |
| Arquivos criados/alterados/renomeados | `ReadDirectoryChangesW`                                 | Watch por diretório. Não associa diretamente com o processo. |
| Pasta monitorada                      | Ex: `C:\Windows`, `C:\Users\%USERNAME%\AppData`, custom | É configurável                                               |



🌀 3. Modelo de Coleta

- O coletor principal roda em um laço contínuo com intervalos fixos (ex: a cada 1 segundo)

- Cada iteração:
  - Captura a lista atual de processos e atualiza dados de CPU/memória 
  - Verifica eventos de arquivos modificados nas pastas observadas

- Os dados são agregados em estruturas internas para alimentar:
  - A baseline (fase de aprendizado)
  - O detector de anomalias

O sistema deve ser capaz de rodar por horas sem causar impacto perceptível.

## Limitações conhecidas

- Nem todos os processos são acessíveis sem privilégio elevado
- Estimativa de CPU é aproximada