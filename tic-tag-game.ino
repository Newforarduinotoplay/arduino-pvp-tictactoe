#include <LedControl.h>
#include <SoftwareSerial.h>

// --- PIN CONFIGURATION & LIBRARIES ---
SoftwareSerial redPVP(8, 9); // Pin 8 (RX), Pin 9 (TX)
LedControl matrix = LedControl(12, 11, 10, 1);

const int pinX = A0;
const int pinY = A1;
const int pinBtn = 2;

// --- GAME VARIABLES ---
int tablero[3][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

int cursorX = 0;
int cursorY = 0;

enum ModoJuego { SELECCION_MODO, TURNO_HUMANO, TURNO_IA, TURNO_RED, FIN_JUEGO };
ModoJuego estadoJuego = SELECCION_MODO;

enum TipoJuego { NINGUNO, IA_VS_HUMAN, HUMAN_VS_HUMAN };
TipoJuego tipoSeleccionado = NINGUNO;

bool esMaestro = true;

void setup() {
  Serial.begin(9600);
  redPVP.begin(9600);
  pinMode(pinBtn, INPUT_PULLUP);

  matrix.shutdown(0, false);
  matrix.setIntensity(0, 4);
  matrix.clearDisplay(0);

  randomSeed(analogRead(A2));

  Serial.println("--- TIC-TAC-TOE ARDUINO PVP / AI ---");
  comprobarRedYRoles();
}

void loop() {
  switch (estadoJuego) {
    case SELECCION_MODO:
      menuSeleccionModo();
      break;
      
    case TURNO_HUMANO:
      gestionarCursorYSeleccion();
      break;
      
    case TURNO_IA:
      Serial.println("AI turn...");
      delay(600);
      turnoIA();
      actualizarPantallaLED();
      verificarFinJuego();
      if (estadoJuego != FIN_JUEGO) estadoJuego = TURNO_HUMANO;
      break;
      
    case TURNO_RED:
      escucharRedPVP();
      break;
      
    case FIN_JUEGO:
      delay(3000);
      reiniciarJuego();
      break;
  }
}

void comprobarRedYRoles() {
  redPVP.println("PING_HOST");
  delay(400);

  if (redPVP.available()) {
    String resp = redPVP.readStringUntil('\n');
    resp.trim();
    if (resp == "PING_HOST" || resp == "SOY_HOST") {
      Serial.println("Two HOSTS detected! Press button within 4s to be SLAVE...");
      unsigned long t = millis();
      while (millis() - t < 4000) {
        if (digitalRead(pinBtn) == LOW) {
          esMaestro = false;
          redPVP.println("SOY_ESCLAVO");
          Serial.println("Configured as SLAVE (Player 2).");
          delay(500);
          break;
        }
      }
    }
  }
  if (esMaestro) {
    redPVP.println("SOY_HOST");
    Serial.println("Configured as MASTER HOST.");
  }
}

void menuSeleccionModo() {
  Serial.println("Move Joystick UP (AI Mode) or DOWN (PvP Network Pin 9)...");
  int valY = analogRead(pinY);
  
  if (valY < 200) {
    tipoSeleccionado = IA_VS_HUMAN;
    estadoJuego = TURNO_HUMANO;
    Serial.println("-> Mode selected: Human vs AI");
    delay(500);
  } else if (valY > 800) {
    tipoSeleccionado = HUMAN_VS_HUMAN;
    estadoJuego = TURNO_HUMANO;
    Serial.println("-> Mode selected: Human vs Human (Pin 9)");
    delay(500);
  }
  delay(200);
}

void gestionarCursorYSeleccion() {
  int valX = analogRead(pinX);
  int valY = analogRead(pinY);

  if (valX > 800) { cursorX = min(cursorX + 1, 2); delay(200); }
  else if (valX < 200) { cursorX = max(cursorX - 1, 0); delay(200); }

  if (valY > 800) { cursorY = min(cursorY + 1, 2); delay(200); }
  else if (valY < 200) { cursorY = max(cursorY - 1, 0); delay(200); }

  actualizarPantallaLED();
  if (tablero[cursorX][cursorY] == 0) {
    matrix.setLed(0, cursorX * 3, cursorY * 3, millis() % 400 < 200);
  }

  if (digitalRead(pinBtn) == LOW) {
    if (tablero[cursorX][cursorY] == 0) {
      tablero[cursorX][cursorY] = 1;
      actualizarPantallaLED();
      
      if (tipoSeleccionado == HUMAN_VS_HUMAN) {
        redPVP.print(cursorX);
        redPVP.print(",");
        redPVP.println(cursorY);
        Serial.println("Move sent to other Arduino via Pin 9.");
        estadoJuego = TURNO_RED;
      } else {
        verificarFinJuego();
        if (estadoJuego != FIN_JUEGO) {
          if (tipoSeleccionado == IA_VS_HUMAN) {
            estadoJuego = TURNO_IA;
          }
        }
      }
    }
    delay(300);
  }
}

void turnoIA() {
  if (intentarGanarOBloquear(2)) return;
  if (intentarGanarOBloquear(1)) return;
  if (tablero[1][1] == 0) { tablero[1][1] = 2; return; }
  
  while (true) {
    int rx = random(0, 3);
    int ry = random(0, 3);
    if (tablero[rx][ry] == 0) {
      tablero[rx][ry] = 2;
      break;
    }
  }
}

bool intentarGanarOBloquear(int jugador) {
  for (int i = 0; i < 3; i++) {
    if (tablero[i][0] == jugador && tablero[i][1] == jugador && tablero[i][2] == 0) { tablero[i][2] = 2; return true; }
    if (tablero[i][0] == jugador && tablero[i][2] == jugador && tablero[i][1] == 0) { tablero[i][1] = 2; return true; }
    if (tablero[i][1] == jugador && tablero[i][2] == jugador && tablero[i][0] == 0) { tablero[i][0] = 2; return true; }
    
    if (tablero[0][i] == jugador && tablero[1][i] == jugador && tablero[2][i] == 0) { tablero[2][i] = 2; return true; }
    if (tablero[0][i] == jugador && tablero[2][i] == jugador && tablero[1][i] == 0) { tablero[1][i] = 2; return true; }
    if (tablero[1][i] == jugador && tablero[2][i] == jugador && tablero[0][i] == 0) { tablero[0][i] = 2; return true; }
  }
  if (tablero[0][0] == jugador && tablero[1][1] == jugador && tablero[2][2] == 0) { tablero[2][2] = 2; return true; }
  if (tablero[0][0] == jugador && tablero[2][2] == jugador && tablero[1][1] == 0) { tablero[1][1] = 2; return true; }
  if (tablero[1][1] == jugador && tablero[2][2] == jugador && tablero[0][0] == 0) { tablero[0][0] = 2; return true; }
  return false;
}

void escucharRedPVP() {
  if (redPVP.available()) {
    String data = redPVP.readStringUntil('\n');
    data.trim();
    int coma = data.indexOf(',');
    if (coma > 0) {
      int rx = data.substring(0, coma).toInt();
      int ry = data.substring(coma + 1).toInt();
      if (tablero[rx][ry] == 0) {
        tablero[rx][ry] = 2;
        actualizarPantallaLED();
        verificarFinJuego();
        if (estadoJuego != FIN_JUEGO) estadoJuego = TURNO_HUMANO;
      }
    }
  }
}

void verificarFinJuego() {
  int res = verificarEstadoLogica();
  if (res == 1) {
    Serial.println("PLAYER 1 WINS!");
    estadoJuego = FIN_JUEGO;
  } else if (res == 2) {
    Serial.println("AI / PLAYER 2 WINS!");
    estadoJuego = FIN_JUEGO;
  } else if (res == 3) {
    Serial.println("DRAW!");
    estadoJuego = FIN_JUEGO;
  }
}

int verificarEstadoLogica() {
  for (int i = 0; i < 3; i++) {
    if (tablero[i][0] != 0 && tablero[i][0] == tablero[i][1] && tablero[i][1] == tablero[i][2]) return tablero[i][0];
    if (tablero[0][i] != 0 && tablero[0][i] == tablero[1][i] && tablero[1][i] == tablero[2][i]) return tablero[0][i];
  }
  if (tablero[0][0] != 0 && tablero[0][0] == tablero[1][1] && tablero[1][1] == tablero[2][2]) return tablero[0][0];
  if (tablero[0][2] != 0 && tablero[0][2] == tablero[1][1] && tablero[1][1] == tablero[2][0]) return tablero[0][2];

  bool lleno = true;
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 3; y++) {
      if (tablero[x][y] == 0) lleno = false;
    }
  }
  if (lleno) return 3;
  return 0;
}

void actualizarPantallaLED() {
  matrix.clearDisplay(0);
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 3; y++) {
      int lx = x * 3;
      int ly = y * 3;
      if (tablero[x][y] == 1) {
        matrix.setLed(0, lx, ly, true);
        matrix.setLed(0, lx+1, ly+1, true);
      } else if (tablero[x][y] == 2) {
        matrix.setLed(0, lx, ly, true);
        matrix.setLed(0, lx+1, ly, true);
        matrix.setLed(0, lx, ly+1, true);
        matrix.setLed(0, lx+1, ly+1, true);
      }
    }
  }
}

void reiniciarJuego() {
  for (int x = 0; x < 3; x++) {
    for (int y = 0; y < 3; y++) {
      tablero[x][y] = 0;
    }
  }
  matrix.clearDisplay(0);
  estadoJuego = SELECCION_MODO;
  Serial.println("Game restarted! Select mode.");
}
