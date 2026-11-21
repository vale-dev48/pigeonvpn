# UI e Mascotte

- Stati mascotte: Disconnected (colori spenti, occhio chiuso), Connecting (ala, orbita ambra, sguardo), Connected (anelli radar, scia, badge globo, collo iridescente animato), Error (rosso, occhi X, badge !).
- Disegno in coordinate 120×120 scalate; `renderPixmap` per icona tray/finestra.
- Backdrop: gradiente slate + alone che anima il colore (QPropertyAnimation 700 ms).
- Card "vetro" via QSS rgba; property dinamica `vpn` per colori stato (repolish).
- Sparkline: RX ciano, TX viola, asse con inerzia.
