# YAMUNA Bitcoin Miner

> Implementação educacional de mineração Bitcoin para microcontroladores ESP32

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![ESP32](https://img.shields.io/badge/platform-ESP32-green.svg)](https://espressif.com/en/products/socs/esp32)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange.svg)](https://platformio.org/)

**YAMUNA** é um software moderno de mineração Bitcoin projetado para microcontroladores ESP32. Embora não seja economicamente viável para lucro, serve como uma excelente ferramenta educacional para compreender protocolos de mineração Bitcoin, desenvolvimento ESP32 e fundamentos de criptomoedas.

---

## Recursos

### Mineração Central
- **Processamento Multi-core**: Utiliza ambos os núcleos do ESP32 para performance otimizada
- **Protocolo Stratum**: Compatibilidade total com pools de mineração padrão
- **Monitoramento em Tempo Real**: Taxa de hash, temperatura e estatísticas de shares ao vivo
- **SHA-256 Otimizado**: Implementação customizada com processamento em lote
- **Detecção de Shares**: Detecção e submissão automática de shares válidos

### Configuração e Gerenciamento
- **Configuração Web**: Interface intuitiva via navegador para configuração
- **Gerenciamento WiFi**: Manipulação inteligente de conexão com modo AP de fallback
- **Gerenciamento de Pool**: Suporte para múltiplos pools de mineração com presets
- **Pronto para OTA**: Projetado para atualizações de firmware over-the-air
- **Tratamento Robusto de Erros**: Proteção watchdog e recuperação automática

### Suporte de Hardware
- **Placas de Desenvolvimento ESP32**: ESP32-WROOM-32 padrão e variantes
- **M5Stack**: Suporte nativo para dispositivos M5Stack Core
- **Auto-detecção**: Detecção automática de hardware e otimização

---

## Requisitos

### Hardware
- **Placa de Desenvolvimento ESP32** (ESP32-WROOM-32 recomendado)
- **Rede WiFi** para conectividade com pool
- **Cabo USB** para programação e alimentação
- **Opcional**: Dissipador de calor para gerenciamento térmico

### Software
- **PlatformIO** (recomendado) ou Arduino IDE
- **Git** para controle de versão
- **Python** 3.6+ (para PlatformIO)

---

## Início Rápido

### 1. Clonar e Configurar
```bash
# Clonar o repositório
git clone https://github.com/yourusername/yamuna.git
cd yamuna

# Instalar dependências (se usando PlatformIO diretamente)
pio pkg install
```

### 2. Compilar e Gravar
```bash
# Compilar firmware
make build

# Gravar no ESP32
make upload

# Monitorar saída serial
make monitor
```

### 3. Configuração Inicial
1. **Conectar à Rede de Configuração**
   - Rede: `YAMUNA`
   - Senha: `yamuna123`

2. **Abrir Portal de Configuração**
   - URL: `http://192.168.4.1`
   - Configurar credenciais WiFi
   - Definir endereço Bitcoin/usuário
   - Selecionar pool de mineração
   - Escolher quantidade de threads (1-2)

3. **Iniciar Mineração**
   - Dispositivo reinicia automaticamente
   - Conecta ao seu WiFi
   - Inicia mineração imediatamente

---

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

### Configuração de Debug

Controle a verbosidade da saída em `core/src/configs.h`:

```cpp
#define DEBUG 0    // 0=desligado, 1=modo desenvolvimento
#define VERBOSE 0  // 0=limpo, 1=mensagens detalhadas
```

**Modos de Saída:**
- `VERBOSE=0`: Saída de produção limpa com informações essenciais
- `VERBOSE=1`: Comunicação detalhada com pool e mensagens operacionais
- `DEBUG=1`: Debug completo de desenvolvimento com detalhes técnicos

---

## Sistema de Build

### Comandos Make
```bash
# Comandos principais
make build          # Compilar firmware
make upload         # Gravar no ESP32
make monitor        # Iniciar monitor serial
make flash          # Build + upload em um passo

# Manutenção
make clean          # Limpar artefatos de build
make deps           # Instalar dependências
make erase          # Apagar flash completamente

# Desenvolvimento
make check          # Executar análise de código
make detect         # Auto-detectar placas ESP32
make info           # Mostrar informações do projeto

# Builds específicos por placa
make BOARD=esp32 build      # Placa de desenvolvimento ESP32
make BOARD=m5stack build    # Dispositivos M5Stack
```

### Ambientes PlatformIO
```ini
# Builds de release (otimizados)
esp32-release       # Placas de desenvolvimento ESP32
m5stack-release     # Dispositivos M5Stack

# Builds de debug (com símbolos)
esp32-debug         # ESP32 com debugging
m5stack-debug       # M5Stack com debugging
```

---

## Saída da Mineração

### Operação Normal
```
YAMUNA Miner v1.0
Bitcoin mining powered by ESP32

WiFi connected!
IP address: 192.168.1.100
Pool: public-pool.io:21496
Address: bc1qexample...
Threads: 2

Authorization successful
Job received - starting mining

>>> Shares: 3 | Hashes: 2847296 | Avg: 24.32 KH/s | Current: 25.1 KH/s | Temp: 56.2°C
Half-share found! Hash: 8bec30dd792358f1...
SHARE FOUND! Hash: 00000012a4c5f832...
Worker[0]: Share found! nonce: 1847263
```

### Monitoramento de Performance
O sistema fornece estatísticas em tempo real a cada 5 segundos:
- **Shares**: Total de shares válidos encontrados
- **Hashes**: Total de cálculos de hash realizados
- **Taxa Média**: Taxa de hash média desde a inicialização
- **Taxa Atual**: Taxa de hash instantânea
- **Temperatura**: Temperatura interna do ESP32

---

## Estrutura do Projeto

```
yamuna/
├── core/                    # Firmware principal
│   ├── src/                 # Código fonte
│   │   ├── main.cpp         # Lógica principal de mineração
│   │   ├── webconfig.cpp    # Configuração web
│   │   ├── webconfig.h      # Cabeçalhos da interface web
│   │   └── configs.h        # Constantes de configuração
│   └── platformio.ini       # Configuração PlatformIO
├── scripts/                 # Automação de build
│   ├── build.sh             # Automação de build
│   ├── detect_board.sh      # Detecção de hardware
│   └── pio_check.sh         # Validação PlatformIO
├── Makefile                 # Sistema de build
├── README.md                # Documentação em inglês
└── README-PT.md             # Este arquivo
```

---

## Recursos de Segurança

### Proteções Integradas
- **Proteção contra Buffer Overflow**: Manipulação segura de strings com verificação de limites
- **Gerenciamento de Memória**: Limpeza automática e prevenção de vazamentos
- **Validação de Entrada**: Validação abrangente de parâmetros
- **Verificações de Ponteiro Nulo**: Proteção contra acesso a memória inválida
- **Timer Watchdog**: Recuperação automática de travamentos do sistema

### Segurança de Rede
- **Timeout de Conexão**: Previne conexões travadas
- **Lógica de Retry**: Backoff exponencial para conexões falhadas
- **Validação DNS**: Resolução segura de hostname
- **Pronto para SSL**: Preparado para conexões criptografadas com pool

---

## Avisos Importantes

### Apenas Propósito Educacional
**Mineração ESP32 NÃO é economicamente viável.** Mineradores ASIC modernos são milhões de vezes mais eficientes. Este projeto é projetado para:

- **Aprender Protocolos Bitcoin**: Compreender algoritmos de mineração e protocolo Stratum
- **Desenvolvimento ESP32**: Explorar programação de microcontroladores e conceitos IoT
- **Educação em Criptomoedas**: Experiência prática com tecnologia blockchain
- **Projetos de Pesquisa**: Casos de uso acadêmicos e experimentais

### Considerações de Hardware
- **Consumo de Energia**: Operação contínua de 1.5-2.5W
- **Geração de Calor**: Garanta refrigeração adequada e ventilação
- **Estresse de Componentes**: Operação estendida pode reduzir vida útil do hardware
- **Fonte de Alimentação**: Use fonte USB de qualidade para estabilidade

### Realidade Econômica
- **Sem Lucro**: Mineração ESP32 gerará recompensas Bitcoin negligíveis
- **Custos de Eletricidade**: Custos de energia excederão qualquer ganho potencial
- **Custo de Oportunidade**: Hardware poderia ser usado para propósitos mais produtivos

---

## Solução de Problemas

### Problemas Comuns

#### Problemas de Conectividade
**Sem Conexão WiFi**
- Verifique SSID/senha na configuração web
- Certifique-se de usar WiFi 2.4GHz (ESP32 não suporta 5GHz)
- Verifique força do sinal WiFi
- Tente reinicializar as configurações: conecte ao AP `YAMUNA`

**Falha na Conexão com Pool**
- Verifique URL e porta do pool na configuração
- Teste conectividade de rede: ping para o pool
- Verifique configurações de firewall/proxy
- Tente um pool diferente da lista de presets

#### Problemas de Performance
**Taxa de Hash Baixa**
- Verifique se a frequência da CPU está em 240MHz
- Monitore temperatura (throttling em temperaturas altas >70°C)
- Tente modo single-thread para estabilidade
- Verifique qualidade da fonte de alimentação

**Travamentos Frequentes**
- Verifique temperatura e adicione dissipador de calor
- Use fonte USB de qualidade (2A mínimo)
- Reduza número de threads para 1
- Verifique configuração do watchdog

#### Problemas de Mineração
**Nenhum Share Encontrado**
- Normal para mineradores pequenos (shares são probabilísticos)
- Verifique dificuldade do pool (use pools para pequenos mineradores)
- Verifique formato do endereço Bitcoin/usuário
- Monitore logs para erros de autorização

**Shares Rejeitados**
- Verifique sincronização de tempo (ESP32 deve ter tempo correto)
- Verifique latência de rede para o pool
- Verifique se endereço Bitcoin está correto
- Tente pool com menor dificuldade

---

## Contribuindo

Damos as boas-vindas a contribuições para melhorar o YAMUNA! Aqui estão algumas áreas de interesse:

### Alta Prioridade
- Otimizações de performance e melhorias no algoritmo de mineração
- Suporte adicional a protocolos de pool de mineração
- Interface web aprimorada com dashboards em tempo real
- Gerenciamento de energia e otimização térmica

### Prioridade Média
- Suporte para variantes e placas ESP32 adicionais
- Mecanismos aprimorados de tratamento de erros e recuperação
- Opções de configuração avançadas e estratégias de mineração
- Melhorias na documentação e tutoriais

### Baixa Prioridade
- Refatoração de código e melhorias arquiteturais
- Testes unitários e framework de testes automatizados
- Internacionalização e localização
- Recursos avançados de análise e logging

### Fluxo de Desenvolvimento
1. Faça fork do repositório
2. Crie uma branch de feature (`git checkout -b feature/recurso-incrivel`)
3. Faça suas mudanças com commits claros e descritivos
4. Adicione testes se aplicável
5. Atualize a documentação
6. Submeta um pull request com descrição detalhada

---

## Licença

Este projeto é open source e disponível sob a [Licença MIT](LICENSE).

---

## Suporte e Comunidade

### Obtendo Ajuda
- **Relatórios de Bug**: [GitHub Issues](https://github.com/yourusername/yamuna/issues)
- **Solicitações de Recurso**: [GitHub Discussions](https://github.com/yourusername/yamuna/discussions)
- **Documentação**: [Wiki do Projeto](https://github.com/yourusername/yamuna/wiki)

### Diretrizes da Comunidade
- Seja respeitoso e construtivo em todas as interações
- Pesquise issues existentes antes de criar novos
- Forneça informações detalhadas ao reportar problemas
- Siga o código de conduta em todos os espaços da comunidade

---

## Agradecimentos

- **Satoshi Nakamoto** - Por criar o Bitcoin e inspirar este projeto
- **Espressif Systems** - Pela excelente plataforma ESP32
- **Comunidade Bitcoin** - Por protocolos abertos e recursos educacionais
- **Contribuidores** - Todos que ajudam a melhorar este projeto

---

<div align="center">

**Alimentado por ESP32 | Construído para Bitcoin | Feito para Aprender**

*Boa Mineração! (Falando educacionalmente)*

</div>