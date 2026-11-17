# Resumo 

- Tô usando UDP porque TCP tava com overhead muito alto em função da quantidade de handshakes excessiva, até tentei usar uma conexão fixa com somente um handshake inicial, mas dá pau porque se o wifi cai de algum lado, o outro dispositivo fica moscando até descobrir. 

- Segundo aquele doc do professor, não dá de só colocar o mesmo SSID e senha pra fechar a topologia. Assim, R1 e R2 precisam ser redes diferentes. 

- Pra fazer o teste S1 -> R1 -> R2 -> Coletor, dá uma lida no .txt que eu larguei por aí. 

- Tem que colocar o pc em modo Hotspot com nome coletor e senha coletor123 e rodar o coletor.py  
