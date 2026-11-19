@ Daily Steven: escena de la sala de Bruno (sustituye al combate inmediato).
@ Todos los textos están en include/dailymax_texts.h; aquí solo hay órdenes.
@ Se incluye desde data/event_scripts.s (lo hace scripts/patch-dailymaximo-hooks.py).
@
@ Orden de la escena:
@  1. Entrada de HORTE por la puerta (sin control)      -> DailyMax_BrunosRoom_EventScript_Enter
@  2. Tienes el control; solo está el jefe              -> (juego normal)
@  3. Hablas con el jefe                                -> DailyMax_BrunosRoom_EventScript_Talk
@  4. Se abre la puerta del fondo y entra el compañero  -> sigue en Talk
@  5. Combate (aquí se gasta el intento del día)        -> special DailyMax_OnBattleStart
@  6. Resultado guardado, mensaje final y fundido       -> DailyMax_BrunosRoom_EventScript_Finish

@ Números de frase: mantener igual que enum DAILYMAX_SPEECH_* en src/dailymaximo.c
	.set DAILYMAX_SPEECH_BOSS_1, 0
	.set DAILYMAX_SPEECH_BOSS_2, 1
	.set DAILYMAX_SPEECH_PARTNER_ENTER, 2
	.set DAILYMAX_SPEECH_PARTNER_BESIDE, 3

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
	closemessage
	@ 4. La puerta del fondo se abre y aparece el compañero del día
	playse SE_DOOR
	call PokemonLeague_EventScript_SetDoorOpen
	special DrawWholeMapView
	clearflag FLAG_HOL_HIDE_TRAINER_B
	addobject LOCALID_HOL_TRAINER_B
	delay 30
	setvar VAR_0x8004, DAILYMAX_SPEECH_PARTNER_ENTER
	special DailyMax_BufferSpeech
	msgbox gStringVar4, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_HOL_TRAINER_B, DailyMax_BrunosRoom_Movement_PartnerWalk
	waitmovement LOCALID_HOL_TRAINER_B
	setvar VAR_0x8004, DAILYMAX_SPEECH_PARTNER_BESIDE
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
