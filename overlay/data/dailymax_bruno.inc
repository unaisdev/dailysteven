@ Daily Steven: escena de la sala de Bruno (sustituye al combate inmediato).
@ Todos los textos están en include/dailymax_texts.h; aquí solo hay órdenes.
@ Se incluye desde data/event_scripts.s (lo hace scripts/patch-dailymaximo-hooks.py).
@
@ Orden de la escena:
@  1. Entrada de HORTE por la puerta (sin control)      -> DailyMax_BrunosRoom_EventScript_Enter
@  2. Tienes el control; solo está el jefe              -> (juego normal)
@  3. Hablas con el jefe                                -> DailyMax_BrunosRoom_EventScript_Talk
@  4. Se abre la puerta; "!" sobre HORTE y STEVEN; STEVEN se gira; el compañero entra en silencio y se coloca a su lado;
@     STEVEN vuelve a mirar al frente; empieza el diálogo del compañero y las dos réplicas -> sigue en Talk
@  5. Combate (aquí se gasta el intento del día)        -> special DailyMax_OnBattleStart
@  6. Resultado guardado, mensaje final y fundido       -> DailyMax_BrunosRoom_EventScript_Finish

@ Números de frase: mantener igual que enum DAILYMAX_SPEECH_* en src/dailymaximo.c
	.set DAILYMAX_SPEECH_BOSS_1, 0
	.set DAILYMAX_SPEECH_BOSS_2, 1
	.set DAILYMAX_SPEECH_BOSS_3, 2
	.set DAILYMAX_SPEECH_PARTNER_ENTER, 3
	.set DAILYMAX_SPEECH_AFTER_1, 4
	.set DAILYMAX_SPEECH_AFTER_2, 5

@ 1. Entrada: HORTE camina desde la puerta de abajo. Sin control hasta el final.
DailyMax_BrunosRoom_EventScript_Enter::
	call PokemonLeague_EventScript_EnterRoom
	setvar VAR_TEMP_1, 1
	releaseall
	end

@ 3-6. Hablar con el jefe (o con el compañero, si se le habla).
DailyMax_BrunosRoom_EventScript_Talk::
	lock
	faceplayer
	setvar VAR_0x8004, DAILYMAX_SPEECH_BOSS_1
	special DailyMax_BufferSpeech
	msgbox gStringVar4, MSGBOX_DEFAULT
	setvar VAR_0x8004, DAILYMAX_SPEECH_BOSS_2
	special DailyMax_BufferSpeech
	msgbox gStringVar4, MSGBOX_DEFAULT
	setvar VAR_0x8004, DAILYMAX_SPEECH_BOSS_3
	special DailyMax_BufferSpeech
	msgbox gStringVar4, MSGBOX_DEFAULT
	closemessage
	@ 4. La puerta del fondo se abre y aparece el compañero del día (sin hablar)
	playse SE_DOOR
	call PokemonLeague_EventScript_SetDoorOpen
	special DrawWholeMapView
	clearflag FLAG_HOL_HIDE_TRAINER_B
	addobject LOCALID_HOL_TRAINER_B
	delay 20
	@    "!" sobre HORTE y sobre STEVEN, a la vez
	playse SE_PIN
	applymovement LOCALID_PLAYER, DailyMax_BrunosRoom_Movement_Exclaim
	applymovement LOCALID_HOL_TRAINER_A, DailyMax_BrunosRoom_Movement_Exclaim
	waitmovement LOCALID_PLAYER
	waitmovement LOCALID_HOL_TRAINER_A
	@    STEVEN se gira hacia la puerta
	applymovement LOCALID_HOL_TRAINER_A, DailyMax_BrunosRoom_Movement_FaceDoor
	waitmovement LOCALID_HOL_TRAINER_A
	delay 20
	@    El compañero entra y camina hasta colocarse al lado de STEVEN
	applymovement LOCALID_HOL_TRAINER_B, DailyMax_BrunosRoom_Movement_PartnerWalk
	waitmovement LOCALID_HOL_TRAINER_B
	delay 10
	@    STEVEN vuelve a mirar al frente (hacia HORTE)
	applymovement LOCALID_HOL_TRAINER_A, DailyMax_BrunosRoom_Movement_FaceFront
	waitmovement LOCALID_HOL_TRAINER_A
	delay 20
	@    Ahora sí empieza el diálogo
	setvar VAR_0x8004, DAILYMAX_SPEECH_PARTNER_ENTER
	special DailyMax_BufferSpeech
	msgbox gStringVar4, MSGBOX_DEFAULT
	setvar VAR_0x8004, DAILYMAX_SPEECH_AFTER_1
	special DailyMax_BufferSpeech
	msgbox gStringVar4, MSGBOX_DEFAULT
	setvar VAR_0x8004, DAILYMAX_SPEECH_AFTER_2
	special DailyMax_BufferSpeech
	msgbox gStringVar4, MSGBOX_DEFAULT
	closemessage
	@ 5. Combate: aquí se consume el intento del día
	special DailyMax_OnBattleStart
	special BattleSetup_StartHallOfLegendsBattle
	@ 6. VAR_RESULT = TRUE si has ganado. Se guarda YA, antes de cualquier mensaje.
	special DailyMax_OnBattleEnd
	special DailyMax_BufferResult
	msgbox gStringVar4, MSGBOX_DEFAULT
	closemessage
	fadescreen FADE_TO_BLACK
	special DailyMax_FinishDaily
DailyMax_BrunosRoom_EventScript_Hold::
	@ La web cierra el juego; mientras tanto no se puede hacer nada más.
	delay 60
	goto DailyMax_BrunosRoom_EventScript_Hold
	end

@ La puerta del fondo está en (6,2); el jefe está en (6,6); el compañero se coloca en (7,6).
DailyMax_BrunosRoom_Movement_PartnerWalk::
	walk_down
	walk_down
	walk_down
	walk_right
	walk_down
	face_down
	step_end

@ Signo de exclamación ("!") sobre quien lo ejecuta (HORTE y STEVEN)
DailyMax_BrunosRoom_Movement_Exclaim::
	emote_exclamation_mark
	step_end

@ STEVEN mira hacia la puerta del fondo y, después, de nuevo al frente (hacia HORTE)
DailyMax_BrunosRoom_Movement_FaceDoor::
	face_up
	step_end

DailyMax_BrunosRoom_Movement_FaceFront::
	face_down
	step_end
