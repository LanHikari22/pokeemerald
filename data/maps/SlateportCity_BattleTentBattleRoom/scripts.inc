SlateportCity_BattleTentBattleRoom_MapScripts:: @ 8209960
	map_script 3, SlateportCity_BattleTentBattleRoom_MapScript1_209970
	map_script 4, SlateportCity_BattleTentBattleRoom_MapScript2_20999A
	map_script 2, SlateportCity_BattleTentBattleRoom_MapScript2_2099B4
	.byte 0

SlateportCity_BattleTentBattleRoom_MapScript1_209970: @ 8209970
	call SlateportCity_BattleTentBattleRoom_EventScript_209976
	end

SlateportCity_BattleTentBattleRoom_EventScript_209976:: @ 8209976
	checkplayergender
	compare VAR_RESULT, 0
	goto_eq SlateportCity_BattleTentBattleRoom_EventScript_20998E
	compare VAR_RESULT, 1
	goto_eq SlateportCity_BattleTentBattleRoom_EventScript_209994
	return

SlateportCity_BattleTentBattleRoom_EventScript_20998E:: @ 820998E
	setvar VAR_OBJ_GFX_ID_1, 100
	return

SlateportCity_BattleTentBattleRoom_EventScript_209994:: @ 8209994
	setvar VAR_OBJ_GFX_ID_1, 105
	return

SlateportCity_BattleTentBattleRoom_MapScript2_20999A: @ 820999A
	map_script_2 VAR_TEMP_1, 0, SlateportCity_BattleTentBattleRoom_EventScript_2099A4
	.2byte 0

SlateportCity_BattleTentBattleRoom_EventScript_2099A4:: @ 82099A4
	setvar VAR_TEMP_1, 1
	hideobjectat 255, MAP_FALLARBOR_TOWN_BATTLE_TENT_BATTLE_ROOM
	hideobjectat 2, MAP_SLATEPORT_CITY_BATTLE_TENT_BATTLE_ROOM
	end

SlateportCity_BattleTentBattleRoom_MapScript2_2099B4: @ 82099B4
	map_script_2 VAR_TEMP_0, 0, SlateportCity_BattleTentBattleRoom_EventScript_2099BE
	.2byte 0

SlateportCity_BattleTentBattleRoom_EventScript_2099BE:: @ 82099BE
	applymovement 3, SlateportCity_BattleTentBattleRoom_Movement_209A99
	waitmovement 0
	setvar VAR_0x8004, 11
	special sub_81A5E74
	setobjectxyperm 2, 5, 1
	removeobject 2
	addobject 2
	applymovement 2, SlateportCity_BattleTentBattleRoom_Movement_209A9E
	waitmovement 0
	setvar VAR_0x8004, 4
	special sub_81B99B4
	lockall
	msgbox gStringVar4, 4
	waitmessage
	closemessage
	special HealPlayerParty
	setvar VAR_0x8004, SPECIAL_BATTLE_FACTORY
	setvar VAR_0x8005, 0
	special DoSpecialTrainerBattle
	waitstate
	switch VAR_RESULT
	case 1, SlateportCity_BattleTentBattleRoom_EventScript_209A39

SlateportCity_BattleTentCorridor_EventScript_209A1B:: @ 8209A1B
	setvar VAR_0x8004, 2
	setvar VAR_0x8005, 0
	setvar VAR_0x8006, 4
	special sub_81A1780
	special LoadPlayerParty
	warp MAP_SLATEPORT_CITY_BATTLE_TENT_LOBBY, 255, 6, 6
	waitstate

SlateportCity_BattleTentBattleRoom_EventScript_209A39:: @ 8209A39
	setvar VAR_0x8004, 1
	setvar VAR_0x8005, 2
	special sub_81A1780
	addvar VAR_RESULT, 1
	setvar VAR_0x8004, 2
	setvar VAR_0x8005, 2
	copyvar VAR_0x8006, VAR_RESULT
	special sub_81A1780
	switch VAR_RESULT
	case 3, SlateportCity_BattleTentBattleRoom_EventScript_209A7B
@ 	case 3, SlateportCity_BattleTentCorridor_EventScript_209A7B
	setvar VAR_0x8006, 1
	warp MAP_SLATEPORT_CITY_BATTLE_TENT_CORRIDOR, 255, 2, 3
	waitstate

SlateportCity_BattleTentBattleRoom_EventScript_209A7B:: @ 8209A7B
SlateportCity_BattleTentCorridor_EventScript_209A7B:: @ 8209A7B
	setvar VAR_0x8004, 2
	setvar VAR_0x8005, 0
	setvar VAR_0x8006, 3
	special sub_81A1780
	special LoadPlayerParty
	warp MAP_SLATEPORT_CITY_BATTLE_TENT_LOBBY, 255, 6, 6
	waitstate
@ forced stop

SlateportCity_BattleTentBattleRoom_Movement_209A99: @ 8209A99
	walk_up
	walk_up
	walk_up
	walk_in_place_fastest_right
	step_end

SlateportCity_BattleTentBattleRoom_Movement_209A9E: @ 8209A9E
	walk_down
	walk_down
	walk_down
	walk_down
	walk_in_place_fastest_left
	step_end

