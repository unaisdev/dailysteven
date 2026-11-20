#ifndef GUARD_DAILYMAX_TEXTS_H
#define GUARD_DAILYMAX_TEXTS_H

// =============================================================================
//  DAILY STEVEN - TEXTOS Y NOMBRES  (TEXTOS DEFINITIVOS; ESTE ES EL ÚNICO ARCHIVO
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
//   - Para el nombre del jefe dentro de una frase usa DAILYMAX_NAME_BOSS (hoy: STEVEN).
//
//  Límites de longitud:
//   - Fuera de combate (hablar con el jefe, entrada del compañero, resultado):
//     sin límite práctico; cada caja admite 2 líneas de unos 30 caracteres y
//     el resto pasa a la siguiente caja. Recomendado: menos de 160 caracteres.
//   - Dentro de combate (frases de victoria y derrota): la caja también es de
//     2 líneas y el texto se guarda en un búfer de 255 bytes por frase.
//     Recomendado: máximo ~120 caracteres (una frase corta o dos).
//   - Los nombres (STEVEN, AURA, BLASCO): máximo 10 letras. Mayúsculas.
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
#define DAILYMAX_NAME_BOSS    "STEVEN"
#define DAILYMAX_NAME_AURA    "AURA"
#define DAILYMAX_NAME_BLASCO  "BLASCO"

// ----------------------------------------------------------------------------
//  CLASES DE ENTRENADOR (lo que acompaña al nombre en los mensajes de combate)
//  STEVEN lleva la misma clase que GOLD en Daily Gold: «GOLD de la ELITE FOUR de CAMPEONES».
//  Es una cadena propia (no cabe en el campo de clase de 12 caracteres), igual que en Daily Gold.
//  Los compañeros llevan la clase «ASPIRANTE» delante del nombre.
// ----------------------------------------------------------------------------
#define DAILYMAX_TEXT_CLASS_BOSS     "ELITE FOUR de CAMPEONES"
#define DAILYMAX_TEXT_BOSS_WITH_CLASS DAILYMAX_NAME_BOSS " de la ELITE FOUR de CAMPEONES"
#define DAILYMAX_TEXT_CLASS_PARTNER  "ASPIRANTE"

// ----------------------------------------------------------------------------
//  ESCENA DE ENTRADA (sala de Bruno)
//  Orden: STEVEN (1, 2, 3) -> se abre la puerta, "!" sobre HORTE y STEVEN, STEVEN se gira, el compañero
//         entra SIN hablar y se coloca a su lado, STEVEN vuelve a mirar al frente
//         -> el compañero habla (ENTER) -> réplicas (AFTER_1, AFTER_2) -> combate.
//  Cada caja muestra el nombre de quien habla (se decide en dailymaximo.c).
// ----------------------------------------------------------------------------
#define DAILYMAX_TEXT_BOSS_TALK_1 \
    "Vaya, vaya… ¿Qué tenemos por aquí?\p" \
    "No todos los días aparece un entrenador capaz de derrotar a GOLD."
#define DAILYMAX_TEXT_BOSS_TALK_2 \
    "No quiero desanimarte, pero, aunque consiguieras derrotarme,\p" \
    "al siguiente entrenador que te espera\p" \
    "solo han conseguido derrotarlo el excampeón y el vigente campeón."
#define DAILYMAX_TEXT_BOSS_TALK_3 \
    "Pero bueno, no seré yo quien se pierda un buen combate.\p" \
    "Así que basta de cháchara…\p" \
    "¡Enséñame lo que vales!"

// Primera frase del compañero (ya colocado al lado de STEVEN; entra caminando sin decir nada)
#define DAILYMAX_TEXT_AURA_ENTER \
    "Vaya… Nada, que no hay manera.\pHe vuelto a perder."
#define DAILYMAX_TEXT_BLASCO_ENTER \
    "No lo voy a conseguir nunca…"

// Después de ENTER.  AFTER_1 y AFTER_2 son dos cajas seguidas; quién habla en cada una:
//   AURA:   AFTER_1 = AURA     AFTER_2 = STEVEN
//   BLASCO: AFTER_1 = STEVEN   AFTER_2 = BLASCO
#define DAILYMAX_TEXT_AURA_AFTER_1 \
    "¡Vaya! ¿Estabais a punto de combatir?\p¿Puedo unirme?"
#define DAILYMAX_TEXT_AURA_AFTER_2 \
    "… No veo por qué no."
#define DAILYMAX_TEXT_BLASCO_AFTER_1 \
    "No digas tonterías.\pAlgún día alguno de nosotros la derrotará.\p" \
    "¿Por qué no te unes al combate, BLASCO?"
#define DAILYMAX_TEXT_BLASCO_AFTER_2 \
    "Bueno, si no os importa…"

// ----------------------------------------------------------------------------
//  FRASES DURANTE EL COMBATE (primero STEVEN, después el compañero)
//    DEFEAT  = lo que dice ese rival cuando HORTE gana
//    VICTORY = lo que dice ese rival cuando HORTE pierde
// ----------------------------------------------------------------------------
#define DAILYMAX_TEXT_BOSS_DEFEAT \
    "¿Es posible… que de verdad haya aparecido alguien capaz de…?"
#define DAILYMAX_TEXT_BOSS_VICTORY \
    "Es mejor que te rindas…"

#define DAILYMAX_TEXT_AURA_DEFEAT \
    "¡No me lo puedo creer!\pSi ganas también el siguiente combate, ¡me muero!"
#define DAILYMAX_TEXT_AURA_VICTORY \
    "¡Vaya! Eres fuerte, pero no tanto como nosotros."

#define DAILYMAX_TEXT_BLASCO_DEFEAT \
    "¡Guau! Eres más impresionante que AURA y STEVEN.\p¡Y mira que es difícil!"
#define DAILYMAX_TEXT_BLASCO_VICTORY \
    "Gracias por el combate."

// ----------------------------------------------------------------------------
//  FINAL DEL COMBATE
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
