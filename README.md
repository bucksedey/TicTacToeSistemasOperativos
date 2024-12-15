En el proyecto se muestra cómo se implementa el juego de Gato en lenguaje C, utilizando
diferentes técnicas de programación de sistemas operativos. Aquí se describen las etapas
clave en el desarrollo de este programa:

* Inicialización: Se inicializan las variables y estructuras necesarias para el juego, incluyendo la
  creación de un tablero de juego y la configuración de los semáforos y la memoria compartida.
  
* Selección del modo de juego: Se solicita al usuario que elija entre jugar contra otro jugador
  humano o contra el sistema.
  
* Juego en curso: Dependiendo de la opción seleccionada, se inicia una serie de turnos entre los
  jugadores. Cada turno implica solicitar al usuario que proporcione sus movimientos, verificar la
  validez de estos movimientos, actualizar el tablero y verificar las condiciones de victoria. En
  caso de que se juegue contra la computadora, se utiliza el algoritmo Minimax para determinar el
  movimiento óptimo.
  
* Finalización del juego: Una vez que se cumple alguna de las condiciones de victoria, se declara
  el ganador (o un empate) y se limpian todos los recursos utilizados durante el juego.
