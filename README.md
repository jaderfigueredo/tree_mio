# Árvore de Classificação Ótima usando Otimização Inteira Mista
Este trabalho implementa uma Árvore de Classificação Ótima usando Otimização Inteira Mista baseada no artigo Optimal classification trees de BERTSIMAS, D.; DUNN, J.

## Abstract
> Além de modelos de aprendizado de máquina já consolidados,
como Árvore de Classificação e Regressão - CART, estudos recentes também
trouxeram novos modelos como Árvore de Classificação Ótima usando Classificação
Inteira Mista - OCT-MIO, capaz de se ajustar ainda mais aos dados de treinamento
e alcançar maior acurácia em alguns casos. Este trabalho traz a modelagem,
implementação e comparação destes dois modelos, tanto em treinamento, quanto em
teste usando validação cruzada (K-Fold), além de uma análise da interpretabilidade
das árvores e da utilização do OCT-MIO como heurística. Os experimentos utilizam
dados reais e sensíveis como para diagnóstico de nível de estresse, predição para
aprovação de crédito e predição de sucesso acadêmico. Apesar do CART ser um
bom modelo de classificação, foi possível observar que o modelo OCT-MIO é
uma alternativa capaz de obter resultados próximos, iguais ou ainda melhores,
especialmente para árvores de classificação de menor altura, ideais em cenários
onde a interpretabilidade é necessária. Desta forma, o modelo OCT-MIO é capaz
de classificar dados mais corretamente que o CART em árvores de altura mínima
o suficiente para classificar todas as classes de um problema, sem abrir mão da
interpretabilidade

# Citações
Como citar este modelo
####Modelos de aprendizado de máquina para árvore de decisão interpretável: otimização vs heurística
```
@article{figueredo2024modelos,
  title={Modelos de aprendizado de m{\'a}quina para {\'a}rvore de decis{\~a}o interpret{\'a}vel: otimiza{\c{c}}{\~a}o vs heur{\'\i}stica},
  author={FIGUEREDO, Jader Duque},
  year={2024}
}
```

#### Árvore de Classificação Ótima Interpretável para o problema de classificação de níveis de estresse
```
@inproceedings{figueredo2025arvore,
  author    = {FIGUEREDO, Jader Duque and
               CAVALHEIRO, Karen Cristina Soares and
               HOKAMA, Pedro},
  title     = {Árvore de Classificação Ótima Interpretável para o problema de classificação de níveis de estresse},
  booktitle = {Proceedings of the LVII Brazilian Symposium on Operations Research},
  year      = {2025},
  address   = {Gramado},
  publisher = {Galoá},
  url       = {https://proceedings.science/sbpo/sbpo-2025/trabalhos/arvore-de-classificacao-otima-interpretavel-para-o-problema-de-classificacao-de?lang=pt-br},
  urldate   = {2026-10-19}
}
```
