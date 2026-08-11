# Estratégia de Preço do SeedUI (venda)

> Documento de negócio. Define faixas de preço, matemática das taxas e plano
> de lançamento. Câmbio usado: **US$ 1 ≈ R$ 5,10** (ago/2026, Banco Central).
> Revisar os valores em reais sempre que o câmbio variar mais de ~10%.

## 1. Referências de mercado (benchmarks reais)

| Ferramenta | Modelo | Preço |
|---|---|---|
| Affinity Designer 2 (pro, pagamento único) | Licença perpétua | US$ 69,99 (promoções ~US$ 35) |
| RPG in a Box (ferramenta nicho de jogo) | Licença perpétua | US$ 24,99 |
| RPG Maker MZ | Licença (Steam) | US$ 79,99 |
| Construct 3 | Assinatura | ~US$ 99/ano |
| Figma (Profissional) | Assinatura | US$ 12–15/mês |
| Canva Pro | Assinatura | US$ 12,99/mês |

## 2. Faixas de preço (recomendado)

| Faixa | Preço USD | Preço BRL | O que inclui |
|---|---|---|---|
| **Gratuito** | US$ 0 | R$ 0 | 1 tela, 1 modo, componentes básicos, exportar com marca d'água do SeedUI. Funil de entrada. |
| **Pro** | US$ 49 | R$ 199 | Tudo do editor: telas/modos ilimitados, modelos (padrão + "Meus Modelos"), exportação limpa, diretrizes completas, temas, atualizações 1.x. |
| **Studio** | US$ 79 | R$ 349–399 | Pro + suporte prioritário, licença para equipes/multi-projeto, exportação em lote, pacotes de modelos premium, integração futura com a engine. |

Alternativa (se preferir assinatura no futuro): US$ 5–9/mês → R$ 25–45/mês,
com desconto anual. Licença perpétua é o modelo mais fácil de vender no Brasil.

## 3. Matemática das taxas (quanto sobra por venda)

| Plataforma | Taxa aproximada | Venda Pro US$ 49 | Venda Pro R$ 199 |
|---|---|---|---|
| Steam | 30% (+ US$ 100 por jogo/ferramenta, pagos uma vez) | ~US$ 34 | ~R$ 139 |
| itch.io | 0–10% (opcional) + processamento ~3% | ~US$ 43–47 | ~R$ 175–190 |
| Gumroad | 10% + processamento (~3% + US$ 0,30) | ~US$ 42–43 | ~R$ 172–175 |
| Plataformas BR (Hotmart/Kiwify) | ~10–15% + processamento | — | ~R$ 165–180 |

Observações:
- Vender via lojas internacionais (Steam/itch.io/Gumroad) simplifica a
  burocracia fiscal — a plataforma recolhe e repassa.
- Venda direta no Brasil exige emissão (MEI/nota) — consultar um contador.
- Use **precificação regional** (Steam e Gumroad fazem automaticamente) para
  o comprador brasileiro pagar em reais.

## 4. Quanto vender para valer a pena (Pro, US$ 49)

| Vendas | Receita bruta | Receita líquida (~75% após taxas) |
|---|---|---|
| 50 | US$ 2.450 | ~US$ 1.850 (~R$ 9,4 mil) |
| 100 | US$ 4.900 | ~US$ 3.700 (~R$ 18,9 mil) |
| 300 | US$ 14.700 | ~US$ 11.000 (~R$ 56 mil) |
| 1.000 | US$ 49.000 | ~US$ 36.800 (~R$ 188 mil) |

## 5. Plano de lançamento (amarrado aos milestones)

| Milestone | O que fazer |
|---|---|
| Hoje (M02) | Não vender. Esqueleto — produto ainda não vendável. |
| M06 (inspetor + diretrizes) | Beta fechado gratuito para coletar feedback. |
| M09 (modelos) | Beta aberto gratuito (early access). |
| M13 (exportação + relatório) | **Early access pago: US$ 29 / R$ 149**. |
| M14 (qualidade, tema claro, desempenho) | **v1.0: US$ 49 / R$ 199**. Quem comprou no early access mantém a v1.x. |
| Futuro (M15, integração engine) | Faixa Studio US$ 79 / R$ 349–399. |

Regras:
- Preço de lançamento menor atrai as primeiras avaliações; subir depois é normal.
- O gratuito é o funil: usuário cria algo, quer exportar sem marca d'água → Pro.
- Marketing: vídeos de "criar um HUD em 5 minutos", comparação com o fluxo
  atual de codar UI na mão, foco em devs indie BR + internacional.

## 6. Riscos e revisões

- Câmbio: revisar tabela BRL se o dólar variar ±10% (atual: R$ 5,10).
- Concorrência gratuita (Figma tem free tier): o diferencial é **offline,
  WYSIWYG para jogo, telas/modos estilo Blender, diretrizes para IA** e a
  integração futura com a engine do usuário.
- Nicho pequeno: preço maior por usuário, menos usuários. O gratuito + modelos
  prontos (estilo Canva) ajudam a reduzir a barreira de entrada.
