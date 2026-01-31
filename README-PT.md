# YAMUNA Bitcoin Miner

Implementação educacional de mineração Bitcoin para microcontroladores ESP32

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![ESP32](https://img.shields.io/badge/platform-ESP32-green.svg)](https://espressif.com/en/products/socs/esp32)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange.svg)](https://platformio.org/)

## Destaques

- Mineração multi-core usando ambos os núcleos do ESP32 para máxima taxa de hash
- Compatibilidade com protocolo Stratum e pools de mineração padrão
- Estatísticas em tempo real de taxa de hash, temperatura e shares a cada 5 segundos
- Cache de midstate SHA-256 — primeira metade do hash calculada uma vez por job
- Detecção e submissão automática de shares válidos ao pool
- Ajuste automático de dificuldade adaptativa baseado na frequência de shares
- Portal de configuração via navegador para WiFi e configurações de pool
- Gerenciamento inteligente de WiFi com modo AP de fallback automático
- Presets de múltiplos pools de mineração com suporte a pool personalizado
- Timer watchdog e recuperação automática de erros para operação confiável
- Suporte nativo a ESP32-WROOM-32 e M5Stack Core com auto-detecção de hardware

## Sumário

- [Visão Geral](#visão-geral)
- [Pré-requisitos](#pré-requisitos)
- [Instalação](#instalação)
- [Início Rápido](#início-rápido)
- [Uso](#uso)
- [Configuração](#configuração)
- [Estrutura do Projeto](#estrutura-do-projeto)
- [Desenvolvimento](#desenvolvimento)
- [Licença](#licença)

## Visão Geral

YAMUNA é firmware de mineração Bitcoin para microcontroladores ESP32. Não é economicamente viável para lucro; existe como ferramenta educacional para compreender protocolos de mineração Bitcoin, desenvolvimento ESP32 e fundamentos de criptomoedas.

## Pré-requisitos

### Hardware

- **Placa de Desenvolvimento ESP32** (ESP32-WROOM-32 recomendado)
- **Rede WiFi** para conectividade com pool
- **Cabo USB** para programação e alimentação
- **Opcional**: Dissipador de calor para gerenciamento térmico

### Software

- **PlatformIO** (recomendado) ou Arduino IDE
- **Python** 3.6+ (para PlatformIO)

## Instalação

### Compilar a partir do código-fonte

```bash
git clone https://github.com/carlosrabelo/yamuna.git
cd yamuna
make deps
make flash
```

Gravar arquivos web no SPIFFS (necessário para o portal de configuração):

```bash
make upload-fs
```

## Início Rápido

```bash
git clone https://github.com/carlosrabelo/yamuna.git
cd yamuna
make deps
make flash
make upload-fs
make monitor
```

### Configuração Inicial

1. Conecte-se à rede de configuração `YAMUNA` (senha: `yamuna123`)
2. Abra `http://192.168.4.1` e configure WiFi, endereço Bitcoin/usuário, pool e senha do pool (padrão: `x`)
3. O dispositivo reinicia, entra no seu WiFi e inicia a mineração em todos os núcleos disponíveis

## Uso

### Operação Normal

`VERBOSE=0` (padrão):

```
YAMUNA Miner v1.0
...
[  24.32 KH/s] 3 shares, 56.2C, diff 1, job a3f92b
yay!!! Share found!
```

`VERBOSE=1`:

```
=== YAMUNA Miner - Modular Architecture ===
...
Pool: public-pool.io:21496
Address: bc1qexample...
...
>>> Shares: 3 | Hashes: 2847296 | Avg: 24.32 KH/s | Current: 25.1 KH/s | Temp: 56.2°C | Stratum Diff: 1 | Job: a3f92b
Worker[0]: VALID SHARE! nonce: 1847263, difficulty: 1
```

### Monitoramento de Performance

O sistema imprime estatísticas a cada 5 segundos:

- **Taxa de Hash**: KH/s instantâneo e médio
- **Shares**: Total de shares válidos submetidos ao pool
- **Temperatura**: Temperatura interna do ESP32
- **Stratum Diff**: Dificuldade atual atribuída pelo pool
- **Job**: ID do job atual recebido do pool

## Configuração

### Pools de Mineração Suportados

| Pool | URL | Porta | Tipo |
|------|-----|-------|------|
| **Public Pool** (Recomendado) | `public-pool.io` | `21496` | Público |
| **Solo CK Pool** | `solo.ckpool.org` | `3333` | Solo |
| **Personalizado** | Sua URL do pool | Sua porta | Personalizado |

### Perfis de Performance

| Configuração | Taxa de Hash | Potência | Temperatura | Estabilidade |
|--------------|--------------|----------|-------------|--------------|
| **Thread Único** | 13-15 KH/s | ~1.5W | 45-55°C | Excelente |
| **Dual Thread** | 24-26 KH/s | ~2.5W | 55-65°C | Boa |

### Dificuldade Adaptativa

O YAMUNA ajusta automaticamente a dificuldade local de shares com base na frequência com que são encontrados. Configurável em `src/configs.h`:

```cpp
#define SHARE_DIFFICULTY_LEVEL 2      // Nível inicial: 1 (mais fácil) a 5 (mais difícil)
#define MAX_DIFFICULTY_LEVEL 5
#define TARGET_SHARE_INTERVAL 120000  // Alvo: um share a cada 2 minutos
#define ADAPTIVE_DIFFICULTY 1         // 0=fixo, 1=ajuste automático
#define DIFFICULTY_ADJUST_INTERVAL_MS 60000  // Tempo mínimo entre ajustes
```

### Configuração de Debug

Controle a verbosidade da saída em `src/configs.h`:

```cpp
#define DEBUG 0    // 0=desligado, 1=modo desenvolvimento
#define VERBOSE 0  // 0=saída limpa, 1=mensagens detalhadas
```

- `VERBOSE=0`: Saída de produção limpa (estilo cpuminer)
- `VERBOSE=1`: Comunicação detalhada com pool e mensagens operacionais
- `DEBUG=1`: Debug completo de desenvolvimento com detalhes técnicos

Portas e velocidades seriais podem ser sobrescritas em `.env` (veja `.env.example`):

```bash
UPLOAD_PORT=/dev/ttyUSB0
MONITOR_PORT=/dev/ttyUSB0
MONITOR_SPEED=115200
UPLOAD_SPEED=921600
```

## Estrutura do Projeto

```
src/                 # Código fonte do firmware (PlatformIO)
data/                # Arquivos web SPIFFS (HTML do portal de configuração)
test/                # Testes unitários Unity
.make/               # Scripts auxiliares PlatformIO
platformio.ini       # Ambientes e dependências PlatformIO
Makefile             # Orquestração de build (BOARD=esp32|m5stack)
```

## Desenvolvimento

```bash
make build           # Compilar firmware (BOARD=esp32|m5stack)
make upload          # Gravar firmware no dispositivo
make flash           # Compilar e gravar
make upload-fs       # Gravar imagem do filesystem
make monitor         # Abrir monitor serial
make test            # Executar testes unitários
make check           # Executar análise estática
make clean           # Remover artefatos de build
make deps            # Instalar dependências
make detect-port     # Detectar porta USB da placa em .env
make erase           # Apagar flash do dispositivo
make install-pio     # Instalar PlatformIO
make help            # Exibir targets disponíveis
```

## Recursos de Segurança

### Proteções Integradas

- Proteção contra buffer overflow com manipulação de strings com verificação de limites
- Limpeza automática de memória e prevenção de vazamentos
- Validação de entrada e parâmetros
- Verificações de ponteiro nulo contra acesso a memória inválida
- Timer watchdog para recuperação automática de travamentos do sistema

### Segurança de Rede

- Timeouts de conexão para evitar sessões TCP travadas
- Reconexão automática com intervalos fixos em caso de falha no pool ou WiFi
- Validação DNS antes das tentativas de conexão direta

## Avisos Importantes

### Apenas Propósito Educacional

Mineração ESP32 **não** é economicamente viável. Mineradores ASIC modernos são milhões de vezes mais eficientes. Este projeto é projetado para:

- Aprender algoritmos de mineração Bitcoin e o protocolo Stratum
- Explorar programação ESP32 e conceitos IoT
- Educação prática em criptomoedas e blockchain
- Pesquisa acadêmica e experimental

### Considerações de Hardware

- Operação contínua consome cerca de 1.5–2.5W
- Garanta refrigeração adequada e ventilação
- Operação estendida pode reduzir a vida útil do hardware
- Use fonte USB de qualidade para estabilidade

### Realidade Econômica

- Mineração ESP32 gerará recompensas Bitcoin negligíveis
- Custos de eletricidade excederão qualquer ganho potencial

## Contribuição

1. Faça fork do repositório
2. Crie uma branch de feature: `git checkout -b feat/descricao`
3. Commit com Conventional Commits: `git commit -m "feat: add X"`
4. Faça push e abra um pull request

## Agradecimentos

- Satoshi Nakamoto — por criar o Bitcoin e inspirar este projeto
- Espressif Systems — pela plataforma ESP32
- Comunidade Bitcoin — por protocolos abertos e recursos educacionais
- Contribuidores — todos que ajudam a melhorar este projeto

## Licença

Este projeto está licenciado sob a Licença MIT — veja [LICENSE](LICENSE) para detalhes.
