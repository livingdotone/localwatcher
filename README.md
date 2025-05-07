## ✅ 1. Objetivo Geral

Construir um sistema que:
- Monitora continuamente o comportamento da máquina local (processos, CPU, arquivos, etc).
- Aprende o que é "normal" para aquela máquina (baseline comportamental)
- Detecta anomalias em tempo real ou quase real, sem uso de assinaturas ou regras fixas.
- Gera eventos de alerta, exportáveis ou visualizáveis, que ajudem a diagnosticar ameaças, erros ou desvios suspeitos.

O objetivo não é substituir antivírus, mas fornecer telemetria comportamental precisa e personalizada para cada sistema.

## 🔍 2. O que será monitorado (v1)
- Processos
    - Nome, PID, tempo de vida(TTL), árvore(pai/filho), caminho do binário
    - Frequência de execução (por hora, por dia da semana)
    - UID/usuário associado ao processo

- CPU e Memória
  - Uso por processo ao longo do tempo
  - Detecção de spikes súbitos em processos incomuns
- Sistema de Arquivos
  - Criação, modificação e exclusão de arquivos em pastas sensíveis. Ex: System32, AppData
  - Associar alterações a um processo específico, se possível

## 📐 3. Estrutura da Baseline
Modelo simples (v1)

- Dados agregados por processo e horário:
```json
{
  "chrome.exe": {
    "seen_hours": [8, 9, 10, ..., 22],
    "avg_cpu": 12.5,
    "max_cpu": 35.0,
    "launch_frequency": "daily",
    "user": "lucas"
  }
}
```
Regras de baseline:
- Se um processo nunca foi visto, ele é considerado suspeito até “aprender”
- Se um processo já visto tiver comportamento fora do padrão, é tratado como possível anomalia

## 🚨 4. Critérios de Detecção de Anomalias
Exemplos de condições que geram alerta:

| Evento                                        | Condição                       | Alerta? |
| --------------------------------------------- | ------------------------------ | ------- |
| Novo processo                                 | Nunca visto e uso de CPU alto  | ✅       |
| Processo comum                                | Rodando fora do horário padrão | ⚠️      |
| Escrita em pasta sensível                     | Por processo desconhecido      | ✅       |
| Processo com tempo de vida anormalmente longo | Sem precedentes                | ⚠️      |
| CPU total do sistema excede muito a média     | Sinal de atividade anormal     | ⚠️      |

## 🧾 5. Saída esperada
Logs e Alertas (formato JSON)

```json
{
  "timestamp": "2025-05-07T02:00:00Z",
  "type": "anomaly",
  "severity": "high",
  "component": "ProcessMonitor",
  "description": "Unseen process powershell.exe launched with 68% CPU at 02:00AM.",
  "details": {
    "pid": 4120,
    "user": "user",
    "command": "powershell.exe -enc ..."
  }
}
```

## 🧱 6. Escopo e Restrições do MVP (v1)

| Item                                  | Incluir?                       |
| ------------------------------------- | ------------------------------ |
| Processos, CPU, arquivos sensíveis    | ✅                              |
| UI/Web dashboard                      | ❌ (pode ser CLI ou exportável) |
| Baseline por processo simples         | ✅                              |
| Machine learning / modelagem avançada | ❌ (só estatístico inicial)     |
| Envio remoto de alertas               | Opcional                       |
| Rede / USB / login events             | ❌ (posterior)                  |
