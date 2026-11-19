#ifndef GUARD_DAILYMAX_TEXTS_H
#define GUARD_DAILYMAX_TEXTS_H

// =============================================================================
//  DAILY STEVEN - TEXTOS Y NOMBRES  (TODO PROVISIONAL, ESTE ES EL ÚNICO ARCHIVO
//  QUE HAY QUE EDITAR PARA CAMBIAR LO QUE SE DICE EN LA ESCENA Y EN EL FINAL)
// =============================================================================
//
//  Cómo editar:
//   - Cambia el texto entre comillas. Cada línea es una cadena de C normal.
//   - Se puede partir un texto en varias comillas seguidas; se pegan solas:
//         "Primera parte "
//         "segunda parte."
//   - NO hace falta meter saltos de línea: el juego reparte el texto en las
//     dos líneas de la caja sin ayuda. Si quieres forzarlos:
//         \n  = salto de línea dentro de la misma caja
//         \p  = caja nueva (pulsar A para seguir)
//   - Se pueden usar acentos, ñ, ¡ y ¿. Un texto largo se parte solo en varias
//     cajas (con la flecha de «seguir»).
//   - Escribe SIEMPRE el nombre del jefe con DAILYMAX_NAME_BOSS (para poder
//     cambiarlo de MÁXIMO a STEVEN tocando solo una línea).
//
//  Límites de longitud:
//   - Fuera de combate (hablar con el jefe, entrada del compañero, resultado):
//     sin límite práctico; cada caja admite 2 líneas de unos 30 caracteres y
//     el resto pasa a la siguiente caja. Recomendado: menos de 160 caracteres.
//   - Dentro de combate (frases de victoria y derrota): la caja también es de
//     2 líneas y el texto se guarda en un búfer de 255 bytes por frase.
//     Recomendado: máximo ~120 caracteres (una frase corta o dos).
//   - Los nombres (MÁXIMO, AURA, BLASCO): máximo 10 letras. Mayúsculas.
//
//  Cómo se aplica: al compilar con  scripts\build-wsl.ps1  (ver README).
//  El script de enganche lee los tres NOMBRES de este archivo para ponerlos
//  también en los datos de los entrenadores; no cambies el formato de las tres
//  líneas  #define DAILYMAX_NAME_*  .
// =============================================================================

// ----------------------------------------------------------------------------
//  NOMBRES EN PANTALLA
// ----------------------------------------------------------------------------
#define DAILYMAX_NAME_PLAYER  "HORTE"      // el jugador, en todo el juego
#define DAILYMAX_NAME_BOSS    "MÁXIMO"     // para pasarlo a STEVEN: cambiar solo esta línea
#define DAILYMAX_NAME_AURA    "AURA"
#define DAILYMAX_NAME_BLASCO  "BLASCO"

// ----------------------------------------------------------------------------
//  ESCENA DE ENTRADA (sala de Bruno)  - PROVISIONAL
// ----------------------------------------------------------------------------

// Al hablar con el jefe: primera caja de diálogo
#define DAILYMAX_TEXT_BOSS_TALK_1 \
    "Así que has llegado hasta aquí. " \
    "Me alegra ver que los entrenadores como tú siguen buscando a los más fuertes."

// Al hablar con el jefe: segunda caja, justo antes de que entre el compañero
#define DAILYMAX_TEXT_BOSS_TALK_2 \
    "Pero hoy no estarás solo ante mí… " \
    "¡Oigo pasos! Alguien más quiere unirse al combate."

// Frase del compañero al entrar por la puerta del fondo
#define DAILYMAX_TEXT_AURA_ENTER \
    "¡Perdona el retraso! ¡He venido corriendo para ayudar a " DAILYMAX_NAME_BOSS "!"
#define DAILYMAX_TEXT_BLASCO_ENTER \
    "¡Lo siento, llego tarde! ¡He corrido todo el camino para pelear junto a " DAILYMAX_NAME_BOSS "!"

// Frase del compañero al colocarse al lado del jefe, mirándote
#define DAILYMAX_TEXT_AURA_BESIDE \
    "¡Estoy lista! ¡Vamos a pelear juntos!"
#define DAILYMAX_TEXT_BLASCO_BESIDE \
    "¡Esta vez no pienso perder! ¡Allá voy!"

// ----------------------------------------------------------------------------
//  FRASES DURANTE EL COMBATE  - PROVISIONAL
//    DEFEAT  = lo que dice ese rival cuando pierde (te ha derrotado a ese rival)
//    VICTORY = lo que dice ese rival cuando gana (te ha derrotado a ti)
// ----------------------------------------------------------------------------
#define DAILYMAX_TEXT_BOSS_DEFEAT \
    "Increíble… Me has superado. Eres un gran entrenador."
#define DAILYMAX_TEXT_BOSS_VICTORY \
    "Hoy gano yo. Pero tus POKéMON han peleado con todas sus fuerzas."

#define DAILYMAX_TEXT_AURA_DEFEAT \
    "¡Vaya! ¡Eres más fuerte de lo que pensaba!"
#define DAILYMAX_TEXT_AURA_VICTORY \
    "¡Hemos ganado! ¡Mis POKéMON también entrenan muy duro!"

#define DAILYMAX_TEXT_BLASCO_DEFEAT \
    "¡Guau! ¡Eres muy fuerte!"
#define DAILYMAX_TEXT_BLASCO_VICTORY \
    "¿Yo… he ganado? ¡Gracias a todos por darme valor!"

// ----------------------------------------------------------------------------
//  FINAL DEL COMBATE  - PROVISIONAL
//  Se muestra: [titular] + [récord] + [despedida].   {STR_VAR_1} y {STR_VAR_2}
//  son el número de victorias y de combates (no los borres del récord).
// ----------------------------------------------------------------------------
#define DAILYMAX_TEXT_RESULT_WON \
    "¡Has vencido a " DAILYMAX_NAME_BOSS " y a su compañero!"
#define DAILYMAX_TEXT_RESULT_LOST \
    "Todo tu equipo ha caído derrotado…"
#define DAILYMAX_TEXT_RECORD \
    "Récord: {STR_VAR_1} victoria(s) en\n{STR_VAR_2} combate(s)."
#define DAILYMAX_TEXT_OUTRO_WON \
    "¡Vuelve mañana para un nuevo combate!"
#define DAILYMAX_TEXT_OUTRO_LOST \
    "Inténtalo de nuevo mañana."

#endif // GUARD_DAILYMAX_TEXTS_H
